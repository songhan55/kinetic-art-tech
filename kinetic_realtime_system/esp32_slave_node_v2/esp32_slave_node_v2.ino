/*
 * =========================================================================================
 * 🕊️ ESP32 슬레이브 노드 12채널 모터 제어 펌웨어 (v2.0)
 * 
 * [프로젝트 사양서: project_spec2.md 기반]
 * - 하드웨어: Standard ESP32 WROOM-32
 * - 서보 드라이버: PCA9685 16채널 I2C PWM 드라이버 (SDA: GPIO 21, SCL: GPIO 22, 주소: 0x40)
 * - 구동 모터 수: 슬레이브 보드당 12개 (PCA9685 채널 0번 ~ 11번)
 * - 단독 직결: GPIO 18 동시 제어
 * - Wi-Fi: 마스터와 동일한 전용 공유기 접속 (SSID: Artech)
 * - 무선 수신: ESP-NOW 1ms 초고속 패킷 수신 (__attribute__((packed)))
 * - 모터 구동:
 *     * 기준 중립 홈 각도: 150.0°
 *     * 최대 스윙 범위: 130.0° ~ 170.0° (±20.0°)
 *     * 50Hz (20ms) 아날로그 LERP 연속 제어 및 12개 모터 동시 구동
 * =========================================================================================
 */

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
#define SDA_PIN 21
#define SCL_PIN 22
#define SERVO_PIN 18            // 단독 서보 신호선: GPIO 18
#define PCA9685_ADDR 0x40

const uint8_t NUM_SERVOS = 12;  // 슬레이브당 제어 모터 수: 12개 (채널 0 ~ 11)
const float BASE_ANGLE = 150.0; // 기준 중립 홈 포지션: 150.0도

// 50Hz 기준 12비트 PWM 틱 (0.5ms = 102 틱, 2.5ms = 512 틱)
#define PCA_TICKS_MIN 102
#define PCA_TICKS_MAX 512

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);
bool pca9685Found = false;

Servo singleServo;

const char* ssid = "Artech";
const char* password = "123456789";

// 각도(0~180도) -> 12비트 PWM 틱값 변환 (고정밀 토크 보장)
inline uint16_t angleToTicks(float angle) {
  angle = constrain(angle, 0.0f, 180.0f);
  return (uint16_t)(PCA_TICKS_MIN + (angle / 180.0f) * (PCA_TICKS_MAX - PCA_TICKS_MIN));
}

// =========================================================================
// [2. ESP-NOW 수신 패킷 구조체 (32비트 바이트 정렬 일치)]
// =========================================================================
typedef struct __attribute__((packed)) struct_message {
  float score;        // 실시간 국제정세 긴장도 (0 ~ 100)
  float wavePhase;    // 마스터의 정확한 파도 위상 (0.0 ~ 6.28)
  float dampFactor;   // 마스터 초음파 센서 거리 감쇄 계수 K (0.12 ~ 1.00)
  float viewerDist;   // 관람객 실측 거리 (m)
  bool isPaused;      // 긴급 정지 여부
  uint8_t cmd;        // 0: 일반, 1: 자가진단 스윙, 2: 재부팅, 10: 수동 각도
} struct_message;

struct_message rxData;

volatile bool isPaused = false;
volatile bool isManualAngleMode = false;
volatile float manualAngle = 150.0;
volatile float targetScore = 32.0;
volatile float masterPhase = 0.0;
volatile float currentDampFactor = 1.0;
volatile float currentViewerDist = 3.0;
volatile unsigned long lastPacketTime = 0;

float smoothedScore = 32.0;
float currentAmplitude = 8.5;
float currentSpeed = 2.5;
float wavePhase = 0.0;
unsigned long lastMotionUpdate = 0;
unsigned long lastStatusPrint = 0;

// =========================================================================
// [3. ESP-NOW 무선 데이터 수신 콜백 (1ms 즉각 반응)]
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

    // 마스터와 위상(wavePhase) 동기화
    wavePhase = masterPhase;

    if (rxData.cmd == 10) {
      // 마스터 수동 각도 제어 명령
      isManualAngleMode = true;
      manualAngle = rxData.score;
      singleServo.write(constrain((int)manualAngle, 0, 180));
      if (pca9685Found) {
        uint16_t t = angleToTicks(manualAngle);
        for (uint8_t ch = 0; ch < NUM_SERVOS; ch++) pwm.setPWM(ch, 0, t);
      }
    } else if (rxData.cmd == 0) {
      isManualAngleMode = false;
    } else if (rxData.cmd == 1) {
      // 자가진단 스윙
      Serial.println("[🎯 자가진단] 마스터 명령 수신 -> 10개 슬레이브 모터 진단 스윙 실행");
      singleServo.write(165);
      if (pca9685Found) {
        for (uint8_t ch = 0; ch < NUM_SERVOS; ch++) pwm.setPWM(ch, 0, angleToTicks(165));
      }
      delay(300);
      singleServo.write(135);
      if (pca9685Found) {
        for (uint8_t ch = 0; ch < NUM_SERVOS; ch++) pwm.setPWM(ch, 0, angleToTicks(135));
      }
      delay(300);
      singleServo.write((int)BASE_ANGLE);
      if (pca9685Found) {
        for (uint8_t ch = 0; ch < NUM_SERVOS; ch++) pwm.setPWM(ch, 0, angleToTicks(BASE_ANGLE));
      }
    } else if (rxData.cmd == 2) {
      Serial.println("[🔄 원격 재부팅] 슬레이브 재부팅");
      delay(200);
      ESP.restart();
    }
  }
}

// =========================================================================
// [초기화 & 메인 50Hz 서보 제어 루프]
// =========================================================================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // 브라운아웃 방지
  Serial.begin(115200);
  delay(300);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // 1. Wi-Fi STA 모드 초기화 (마스터와 동일 공유기에 연결하여 채널 완벽 일치)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wi-Fi 연결 대기 (ESP-NOW가 동일 채널로 초기화되도록)
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[Wi-Fi] 연결 완료!");

  Serial.println("\n==================================================");
  Serial.println("  🕊️ ESP32 Slave Node Receiver [12개 모터 전용 v2.0]");
  Serial.println("  PCA9685 채널 0~11번 + ESP-NOW 1ms 칼군무 파도 엔진");
  Serial.printf("  내 고유 MAC 주소: %s\n", WiFi.macAddress().c_str());
  Serial.println("==================================================");

  // 2. I2C 및 PCA9685 16채널 드라이버 초기화 (SDA: 21, SCL: 22)
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  if (pwm.begin()) {
    pca9685Found = true;
    pwm.setPWMFreq(50);
    delay(10);

    // MODE2: 토템폴 푸시풀 드라이버 강제 활성화 (OUTDRV=1)
    Wire.beginTransmission(PCA9685_ADDR);
    Wire.write(0x01); // MODE2
    Wire.write(0x04); // OUTDRV=1
    Wire.endTransmission();

    Serial.printf("[PCA9685] 🎉 12채널 서보 드라이버 감지 완료! (50Hz 토템폴 가동)\n");
    uint16_t baseTicks = angleToTicks(BASE_ANGLE);
    for (uint8_t ch = 0; ch < NUM_SERVOS; ch++) {
      pwm.setPWM(ch, 0, baseTicks);
    }
  } else {
    Serial.println("[PCA9685] ⚠️ 드라이버 미감지 -> GPIO 18 단독 서보 모드로 동작");
  }

  // 3. 단독 GPIO 서보 초기화
  singleServo.setPeriodHertz(50);
  singleServo.attach(SERVO_PIN, 500, 2500);
  singleServo.write((int)BASE_ANGLE);

  // 4. ESP-NOW 초기화
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(onDataRecv);
    Serial.println("[ESP-NOW] 수신기 등록 완료! 마스터 신호 대기 중...");
  }
}

void loop() {
  unsigned long now = millis();

  // 50Hz (20ms 주기) 아날로그 LERP 연속 모션 제어 루프
  if (now - lastMotionUpdate >= 20) {
    float dt = (now - lastMotionUpdate) / 1000.0f;
    lastMotionUpdate = now;

    if (!isPaused && !isManualAngleMode) {
      // 부드러운 스코어 LERP
      smoothedScore += (targetScore - smoothedScore) * 0.05f;

      // 점수(0~100)에 따른 진폭 및 속도 프로필
      float targetAmp = 5.0f + (smoothedScore / 100.0f) * 15.0f; // ±5° ~ ±20°
      float targetSpd = 1.5f + (smoothedScore / 100.0f) * 3.5f;

      currentAmplitude += (targetAmp - currentAmplitude) * 0.05f;
      currentSpeed += (targetSpd - currentSpeed) * 0.05f;

      // 관람객 초음파 거리 감쇄 K 반영
      float effectiveAmp = currentAmplitude * currentDampFactor;
      float effectiveSpd = currentSpeed * (0.6f + 0.4f * currentDampFactor);

      // 패킷 수신이 500ms 이상 지연될 경우 자체 위상 진행 유지
      if (now - lastPacketTime > 500) {
        wavePhase += (effectiveSpd * 1.25f) * dt;
        if (wavePhase > 6.2831853f) wavePhase -= 6.2831853f;
      }

      // 채널 0 (또는 단독 서보) 기준 각도 계산
      float targetAngle = BASE_ANGLE + sin(wavePhase) * effectiveAmp;
      targetAngle = constrain(targetAngle, 130.0f, 170.0f);
      singleServo.write((int)targetAngle);

      // 12개 모터 완전 동시 칼군무 (시차 0: 12개 모터가 완전히 일제히 동일 각도로 움직임)
      if (pca9685Found) {
        uint16_t syncTicks = angleToTicks(targetAngle);
        for (uint8_t ch = 0; ch < NUM_SERVOS; ch++) {
          pwm.setPWM(ch, 0, syncTicks);
        }
      }
    } else if (isPaused && !isManualAngleMode) {
      // 긴급 정지 상태: 150도 수직 홈 포지션 유지
      singleServo.write((int)BASE_ANGLE);
      if (pca9685Found) {
        uint16_t baseTicks = angleToTicks(BASE_ANGLE);
        for (uint8_t ch = 0; ch < NUM_SERVOS; ch++) {
          pwm.setPWM(ch, 0, baseTicks);
        }
      }
    }
  }

  // 1초 주기 시리얼 모니터 텔레메트리 출력
  if (now - lastStatusPrint >= 1000) {
    lastStatusPrint = now;
    bool isAlive = (now - lastPacketTime < 1000);
    Serial.printf("[슬레이브 상태] 통신: %s | 스코어: %.1f | K: %.2f | 거리: %.2fm | 모터: 12개 가동 | 정지: %s\n",
      isAlive ? "🟢 1ms 수신 중" : "🔴 마스터 대기",
      targetScore,
      currentDampFactor,
      currentViewerDist,
      isPaused ? "YES (150°)" : "NO"
    );
  }
}
