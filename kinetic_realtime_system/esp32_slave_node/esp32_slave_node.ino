#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 브라운아웃(전압 강하 리셋) 방지 헤더
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// =========================================================================
// [1. 핀 배선 및 모터 파라미터]
// =========================================================================
const int SERVO_PIN = 18;       // 서보 신호선: GPIO 18 (직결용)
const float BASE_ANGLE = 150.0; // 기본 중립 위치: 150도

// I2C 및 PCA9685 16채널 PWM 드라이버 (SDA: GPIO 21, SCL: GPIO 22)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
bool pca9685Found = false;

Servo singleServo;

const char* ssid = "Artech";
const char* password = "123456789";

// 각도(0~180도) -> 마이크로초(500us ~ 2500us) 정밀 변환 함수
inline int angleToMicros(float angle) {
  angle = constrain(angle, 0.0f, 180.0f);
  return (int)(500.0f + (angle / 180.0f) * 2000.0f);
}

// =========================================================================
// [2. ESP-NOW 수신 패킷 구조체]
// =========================================================================
typedef struct __attribute__((packed)) struct_message {
  float score;        // 실시간 국제정세 긴장도 (0 ~ 100)
  float wavePhase;    // 마스터의 정확한 파도 위상 (0.0 ~ 6.28)
  float dampFactor;   // 마스터 초음파 센서 거리 감쇄 계수 K (0.15 ~ 1.00)
  float viewerDist;   // 관람객 실측 거리 (m)
  bool isPaused;      // 긴급 정지 여부
  uint8_t cmd;        // 0: 일반, 1: 자가진단 스윙, 2: 재부팅, 10: 수동 각도
} struct_message;

struct_message rxData;

volatile bool isPaused = false;
volatile bool isManualAngleMode = false;
volatile float manualAngle = 150.0;
volatile float targetScore = 31.5;
volatile float masterPhase = 0.0;
volatile float currentDampFactor = 1.0;
volatile float currentViewerDist = 3.0;
volatile unsigned long lastPacketTime = 0;

float smoothedScore = 31.5;
float smoothedDamp = 1.0;
float currentAmplitude = 8.5;
float currentSpeed = 2.5;
float wavePhase = 0.0;
unsigned long lastMotionUpdate = 0;
unsigned long lastStatusPrint = 0;

// =========================================================================
// [3. ESP-NOW 무선 데이터 수신 콜백 (0.001초 즉각 반응)]
// =========================================================================
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  if (len == sizeof(struct_message)) {
    memcpy(&rxData, incomingData, sizeof(rxData));
    lastPacketTime = millis();

    targetScore = rxData.score;
    masterPhase = rxData.wavePhase;
    currentDampFactor = rxData.dampFactor;
    currentViewerDist = rxData.viewerDist;
    isPaused = rxData.isPaused;

    // 마스터와 위상(wavePhase) 1ms 칼일치 동기화
    wavePhase = masterPhase;

    if (rxData.cmd == 10) {
      // 마스터에서 직접 각도 수동 제어 명령 수신
      isManualAngleMode = true;
      manualAngle = rxData.score;
      singleServo.write(constrain((int)manualAngle, 0, 180));
      if (pca9685Found) {
        for (uint8_t ch = 0; ch < 3; ch++) {
          pwm.writeMicroseconds(ch, angleToMicros(manualAngle));
        }
      }
      Serial.printf("[🎯 마스터 수동 각도 수신] 슬레이브 서보 %.1f° 즉각 회전 완료!\n", manualAngle);
    } else if (rxData.cmd == 0) {
      isManualAngleMode = false;
    } else if (rxData.cmd == 1) {
      Serial.println("[🎯 자가진단] 마스터 명령 수신 -> 슬레이브 진단 스윙 실행");
      singleServo.write(165);
      if (pca9685Found) {
        for (uint8_t ch = 0; ch < 3; ch++) pwm.writeMicroseconds(ch, angleToMicros(165));
      }
      delay(300);
      singleServo.write(135);
      if (pca9685Found) {
        for (uint8_t ch = 0; ch < 3; ch++) pwm.writeMicroseconds(ch, angleToMicros(135));
      }
      delay(300);
      singleServo.write(BASE_ANGLE);
      if (pca9685Found) {
        for (uint8_t ch = 0; ch < 3; ch++) pwm.writeMicroseconds(ch, angleToMicros(BASE_ANGLE));
      }
    } else if (rxData.cmd == 2) {
      Serial.println("[🔄 원격 재부팅] 마스터 명령 수신 -> 슬레이브 재부팅");
      delay(200);
      ESP.restart();
    }
  }
}

// =========================================================================
// [초기화 & 메인 50Hz 서보 제어 루프]
// =========================================================================
void setup() {
  // 브라운아웃 디텍터 비활성화 (모터 전압 강하 재부팅 방지)
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(300);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // 1. Wi-Fi STA 모드 초기화 (MAC 주소 및 마스터 채널 동기화)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("\n==================================================");
  Serial.println("  ESP32 [슬레이브 노드 수신기] Multi-Slave 지원");
  Serial.println("  ESP-NOW MAC 무선 1ms 직결 수신 엔진 (PCA9685 + GPIO18)");
  Serial.printf("  내 고유 MAC 주소: %s\n", WiFi.macAddress().c_str());
  Serial.println("==================================================");

  // 2. I2C 및 PCA9685 16채널 드라이버 초기화 시도 (SDA: 21, SCL: 22)
  Wire.begin(21, 22);
  if (pwm.begin()) {
    pca9685Found = true;
    pwm.setPWMFreq(50);
    Serial.println("[PCA9685] 🎉 16채널 I2C 서보 드라이버 감지 완료! 50Hz 가동");
    for (uint8_t ch = 0; ch < 3; ch++) {
      pwm.writeMicroseconds(ch, angleToMicros(BASE_ANGLE));
    }
  } else {
    Serial.println("[PCA9685] ℹ️ PCA9685 미연결 -> GPIO 18 단독 직결 모드로 가동");
  }

  // 3. 단독 GPIO 18 서보 초기화
  singleServo.setPeriodHertz(50);
  singleServo.attach(SERVO_PIN, 500, 2500);
  singleServo.write(BASE_ANGLE); // 150도 중립 정지 대기
  delay(100);

  // 4. ESP-NOW 무선 수신 초기화
  if (esp_now_init() == ESP_OK) {
    Serial.println("[ESP-NOW] 🎉 슬레이브 무선 수신 대기 시작 (1ms 패킷 수신 대기)");
    esp_now_register_recv_cb(onDataRecv);
  } else {
    Serial.println("[ESP-NOW] ❌ 초기화 실패!");
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. 점수 및 마스터 초음파 감쇄 계수(K) 스무딩
  smoothedScore = smoothedScore * 0.85f + targetScore * 0.15f;
  smoothedDamp = smoothedDamp * 0.80f + currentDampFactor * 0.20f;
  currentSpeed = 1.5f + (smoothedScore / 100.0f) * 3.5f;

  // 2. 상태 주기적 시리얼 출력 (2초마다)
  if (currentMillis - lastStatusPrint >= 2000) {
    lastStatusPrint = currentMillis;
    bool isConnected = (currentMillis - lastPacketTime < 2000);
    Serial.printf("[🛰️ 슬레이브 상태] 통신: %s | 점수: %.1f | 진폭: ±%.1f° | 감쇄 K: %.2f | PCA: %s\n",
      isConnected ? "🟢 1ms 수신 중" : "🔴 마스터 대기",
      smoothedScore, currentAmplitude, smoothedDamp, pca9685Found ? "OK" : "GPIO18");
  }

  // 3. 초당 50회(20ms) 마스터와 100% 동기화된 연속 서보 구동 & 프레임 LERP
  if (currentMillis - lastMotionUpdate >= 20) {
    lastMotionUpdate = currentMillis;

    if (isPaused) {
      singleServo.write(BASE_ANGLE); // 150도 중립 안전 고정
      if (pca9685Found) {
        for (uint8_t ch = 0; ch < 3; ch++) {
          pwm.writeMicroseconds(ch, angleToMicros(BASE_ANGLE));
        }
      }
    } else if (isManualAngleMode) {
      int target = constrain((int)manualAngle, 0, 180);
      singleServo.write(target);
      if (pca9685Found) {
        for (uint8_t ch = 0; ch < 3; ch++) {
          pwm.writeMicroseconds(ch, angleToMicros(target));
        }
      }
    } else {
      // 50Hz 프레임 단위 연속 진폭 보간
      float baseAmp = 5.0f + (smoothedScore / 100.0f) * 15.0f;
      float targetAmp = baseAmp * smoothedDamp;
      currentAmplitude += (targetAmp - currentAmplitude) * 0.08f;

      wavePhase += 0.025f * currentSpeed;
      if (wavePhase > 6.28318f * 100.0f) wavePhase = 0.0f;

      // 단독 GPIO 18 서보
      float angleMain = BASE_ANGLE + sin(wavePhase) * currentAmplitude;
      angleMain = constrain(angleMain, 130.0f, 170.0f);
      singleServo.write(angleMain);

      // PCA9685 3채널 다채널 확장 (공간 위상차)
      if (pca9685Found) {
        float angle0 = BASE_ANGLE + sin(wavePhase) * currentAmplitude;
        float angle1 = BASE_ANGLE + sin(wavePhase - 0.25f) * currentAmplitude;
        float angle2 = BASE_ANGLE + sin(wavePhase - 0.50f) * currentAmplitude;

        pwm.writeMicroseconds(0, angleToMicros(constrain(angle0, 130.0f, 170.0f)));
        pwm.writeMicroseconds(1, angleToMicros(constrain(angle1, 130.0f, 170.0f)));
        pwm.writeMicroseconds(2, angleToMicros(constrain(angle2, 130.0f, 170.0f)));
      }
    }
  }
}
