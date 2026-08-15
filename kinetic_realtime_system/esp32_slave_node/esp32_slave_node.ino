#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ESP32Servo.h>

// 브라운아웃(전압 강하 리셋) 방지 헤더
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// =========================================================================
// [1. 핀 배선 및 모터 파라미터]
// =========================================================================
const int SERVO_PIN = 18;       // 서보 신호선: GPIO 18
const float BASE_ANGLE = 150.0; // 기본 중립 위치: 150도

Servo singleServo;

const char* ssid = "MIRR";
const char* password = "mirr3411";

// =========================================================================
// [2. ESP-NOW 수신 패킷 구조체]
// =========================================================================
typedef struct struct_message {
  float score;        // 실시간 소셜 지수 (0 ~ 100)
  float wavePhase;    // 마스터의 정확한 파도 위상 (0.0 ~ 6.28)
  bool isPaused;      // 긴급 정지 여부
  uint8_t cmd;        // 0: 일반, 1: 자가진단 스윙, 2: 재부팅
} struct_message;

struct_message rxData;

volatile bool isPaused = false;
volatile float targetScore = 42.0;
volatile float masterPhase = 0.0;
volatile unsigned long lastPacketTime = 0;

float smoothedScore = 42.0;
float currentAmplitude = 11.3;
float currentSpeed = 2.9;
float wavePhase = 0.0;
unsigned long lastMotionUpdate = 0;

// =========================================================================
// [3. ESP-NOW 무선 데이터 수신 콜백 (0.001초 즉각 반응)]
// =========================================================================
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  if (len == sizeof(struct_message)) {
    memcpy(&rxData, incomingData, sizeof(rxData));
    lastPacketTime = millis();

    targetScore = rxData.score;
    masterPhase = rxData.wavePhase;
    isPaused = rxData.isPaused;

    // 마스터와 위상(wavePhase) 1ms 칼일치 동기화
    wavePhase = masterPhase;

    if (rxData.cmd == 1) {
      Serial.println("[🎯 자가진단] 마스터 명령 수신 -> 슬레이브 진단 스윙 실행");
      singleServo.write(165);
      delay(300);
      singleServo.write(135);
      delay(300);
      singleServo.write(BASE_ANGLE);
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

  Serial.println("\n==================================================");
  Serial.println("  ESP32 [슬레이브 수신기] A to C 보드");
  Serial.println("  ESP-NOW MAC 무선 1ms 직결 수신 엔진");
  Serial.printf("  내 고유 MAC 주소: %s\n", WiFi.macAddress().c_str());
  Serial.println("==================================================");

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  singleServo.setPeriodHertz(50);
  singleServo.attach(SERVO_PIN, 500, 2500);

  // 부팅 진단 스윙
  singleServo.write(165);
  delay(250);
  singleServo.write(135);
  delay(250);
  singleServo.write(BASE_ANGLE);
  delay(250);

  // 마스터와 동일한 Wi-Fi 채널 동기화를 위해 Wi-Fi 연결
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // ESP-NOW 초기화
  if (esp_now_init() == ESP_OK) {
    Serial.println("[ESP-NOW] 🎉 슬레이브 무선 수신 대기 시작!");
    esp_now_register_recv_cb(onDataRecv);
  } else {
    Serial.println("[ESP-NOW] 초기화 실패!");
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. 점수 스무딩 & 진폭 계산
  smoothedScore = smoothedScore * 0.85f + targetScore * 0.15f;
  if (isPaused) {
    currentAmplitude = 0.0;
  } else {
    currentAmplitude = 5.0f + (smoothedScore / 100.0f) * 15.0f;
    currentSpeed = 1.5f + (smoothedScore / 100.0f) * 3.5f;
  }

  // 2. 초당 50회(20ms) 마스터와 100% 동기화된 연속 서보 구동
  if (currentMillis - lastMotionUpdate >= 20) {
    lastMotionUpdate = currentMillis;

    if (isPaused) {
      singleServo.write(BASE_ANGLE); // 150도 중립 안전 고정
    } else {
      wavePhase += 0.025f * currentSpeed;
      if (wavePhase > 6.28318f * 100.0f) wavePhase = 0.0f;

      float targetAngle = BASE_ANGLE + sin(wavePhase) * currentAmplitude;
      targetAngle = constrain(targetAngle, 130.0f, 170.0f);
      singleServo.write(targetAngle);
    }
  }
}
