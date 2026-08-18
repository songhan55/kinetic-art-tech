#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 브라운아웃(모터 전압 강하 리셋) 방지 헤더
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// =========================================================================
// [1. PCA9685 16채널 I2C 서보 드라이버 설정]
// =========================================================================
#define SDA_PIN 21
#define SCL_PIN 22
#define PCA9685_ADDR 0x40

// 8개 채널(0번 ~ 7번) 전체 동시 지원 (채널 번호 착오 방지)
#define NUM_CHANNELS 8

// MG996R 서보 표준 12비트 PWM 펄스 범위 (50Hz 기준, 20ms = 4096 ticks)
// 500us (0도)  = (500 / 20000) * 4096 = 102.4 -> 102
// 2500us (180도) = (2500 / 20000) * 4096 = 512.0 -> 512
#define SERVOMIN 102 // 0도
#define SERVOMAX 512 // 180도

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);
bool pca9685Found = false;

const float BASE_ANGLE = 150.0; // 기준 중립 위치: 150도
float currentAngles[NUM_CHANNELS];

// 동작 모드 상태
enum Mode {
  MODE_MANUAL,    // 직접 각도 지정 고정 모드
  MODE_WAVE,      // 연속 파도타기 모드
  MODE_SWEEP      // 왕복 스위프 모드
};

Mode currentMode = MODE_MANUAL;
float wavePhase = 0.0;
float waveSpeed = 2.5;
float waveAmplitude = 15.0; // ±15도 스윙 (135° ~ 165°)
unsigned long lastMotionUpdate = 0;

// 각도(0~180도) -> PCA9685 12비트 정밀 펄스(102~512) 변환 (직접 연산으로 라이브러리 오차 완전 제거)
inline int angleToPulse(float angle) {
  angle = constrain(angle, 0.0f, 180.0f);
  return (int)(SERVOMIN + (angle / 180.0f) * (SERVOMAX - SERVOMIN));
}

void setServoAngle(uint8_t channel, float angle) {
  if (channel >= 16) return;
  angle = constrain(angle, 0.0f, 180.0f);
  if (channel < NUM_CHANNELS) {
    currentAngles[channel] = angle;
  }
  if (pca9685Found) {
    int pulse = angleToPulse(angle);
    pwm.setPWM(channel, 0, pulse);
  }
}

void setAllServos(float angle) {
  for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
    setServoAngle(i, angle);
  }
}

void printMenu() {
  Serial.println(F("\n=================================================="));
  Serial.println(F("  🎮 [ESP32 + PCA9685 12비트 다채널 서보 테스트기]"));
  Serial.println(F("=================================================="));
  Serial.println(F("👉 [전체 각도 제어] 숫자만 입력 후 Enter (채널 0~7 동시 제어)"));
  Serial.println(F("   예: 150    -> 모든 모터 150도(중립 홈)로 즉시 회전"));
  Serial.println(F("   예: 130    -> 모든 모터 130도로 즉시 회전"));
  Serial.println(F("   예: 170    -> 모든 모터 170도로 즉시 회전"));
  Serial.println(F("👉 [개별 채널 제어] '채널 각도'"));
  Serial.println(F("   예: 0 130  -> 0번 채널만 130도로 회전"));
  Serial.println(F("   예: 1 150  -> 1번 채널만 150도로 회전"));
  Serial.println(F("   예: 2 170  -> 2번 채널만 170도로 회전"));
  Serial.println(F("👉 [진단 및 동적 모드]"));
  Serial.println(F("   예: test   -> 0번, 1번, 2번 모터를 하나씩 순서대로 징~ 움직여 채널 자가진단"));
  Serial.println(F("   예: wave   -> 3개 모터가 시차를 두고 출렁이는 연속 파도타기"));
  Serial.println(F("   예: sweep  -> 130도 ~ 170도 사이를 부드럽게 2회 왕복 스위프"));
  Serial.println(F("   예: home   -> 150도 중립 정렬 및 정지"));
  Serial.println(F("==================================================\n"));
}

// 채널별 1개씩 순차 자가진단 함수 (0번 -> 1번 -> 2번 개별 테스트)
void runChannelDiagnostic() {
  Serial.println(F("\n🔍 [자가진단 시작] 0번, 1번, 2번 채널을 순서대로 1개씩 테스트합니다..."));
  for (uint8_t ch = 0; ch < 3; ch++) {
    Serial.printf(">>> [채널 %d번 테스트] 130° -> 170° -> 150° 스윙 중...\n", ch);
    setServoAngle(ch, 130.0);
    delay(400);
    setServoAngle(ch, 170.0);
    delay(400);
    setServoAngle(ch, 150.0);
    delay(300);
  }
  Serial.println(F("✅ [자가진단 완료] 3개 채널 모두 150도 정렬 완료!\n"));
}

// 등속도 왕복 2회 스위프 함수
void runSweepTest(float minAng, float maxAng, int msDelay) {
  Serial.println(F("🚀 [스위프 시작] 130° ~ 170° 왕복 2회 진행 중..."));
  for (int round = 1; round <= 2; round++) {
    for (float a = BASE_ANGLE; a <= maxAng; a += 1.0) {
      setAllServos(a);
      delay(msDelay);
    }
    for (float a = maxAng; a >= minAng; a -= 1.0) {
      setAllServos(a);
      delay(msDelay);
    }
    for (float a = minAng; a <= BASE_ANGLE; a += 1.0) {
      setAllServos(a);
      delay(msDelay);
    }
  }
  Serial.println(F("✅ [스위프 완료] 150도 홈 포지션 복귀 완료!"));
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setTimeout(50); // 50ms 초고속 시리얼 타임아웃 (줄바꿈 설정 없어도 즉각 반응)
  delay(300);

  Serial.println(F("\n[초기화] ESP32 I2C 버스 시작 (SDA: GPIO 21, SCL: GPIO 22)..."));
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000); // 400kHz I2C 고속 모드

  for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
    currentAngles[i] = BASE_ANGLE;
  }

  if (pwm.begin()) {
    pca9685Found = true;
    pwm.setOscillatorFrequency(27000000); // 정밀 내부 클럭 27MHz 보정
    pwm.setPWMFreq(50); // 50Hz 서보 표준 주파수
    delay(10);

    Serial.println(F("🎉 [PCA9685 감지 성공] 16채널 I2C 드라이버 초기화 완료!"));
    // 채널 0 ~ 7 전체를 150도 중립으로 초기화
    setAllServos(BASE_ANGLE);
  } else {
    Serial.println(F("❌ [PCA9685 감지 실패] I2C 배선(SDA 21 / SCL 22) 및 3.3V/GND 전원을 확인해주세요."));
  }

  printMenu();
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. 시리얼 터미널 사용자 입력 처리 (줄바꿈 설정 무관 초고속 처리)
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();
    input.replace(',', ' ');
    input.replace('\r', ' ');
    input.trim();

    if (input.length() > 0) {
      Serial.printf("👉 [명령 수신] \"%s\"\n", input.c_str());

      if (input == "home" || input == "h" || input == "stop") {
        currentMode = MODE_MANUAL;
        setAllServos(BASE_ANGLE);
        Serial.println(F("🎯 [홈 포지션] 모든 서보모터 150도 정렬 및 정지 대기 완료"));
      } else if (input == "test" || input == "t" || input == "diag") {
        currentMode = MODE_MANUAL;
        runChannelDiagnostic();
      } else if (input == "wave" || input == "w") {
        currentMode = MODE_WAVE;
        wavePhase = 0.0;
        Serial.println(F("🌊 [파도 모드 시작] 모터들이 위상차를 두고 유기적으로 출렁입니다! (정지는 'home' 입력)"));
      } else if (input == "sweep" || input == "s") {
        currentMode = MODE_MANUAL;
        runSweepTest(130.0, 170.0, 15);
      } else {
        int spaceIdx = input.indexOf(' ');
        if (spaceIdx != -1) {
          // [개별 채널 제어: '채널 각도' -> 예: 0 130, 1 150, 2 170]
          String chStr = input.substring(0, spaceIdx);
          String angStr = input.substring(spaceIdx + 1);
          chStr.trim();
          angStr.trim();

          int ch = chStr.toInt();
          float ang = angStr.toFloat();
          if (ch >= 0 && ch < 16 && ang >= 0.0 && ang <= 180.0) {
            currentMode = MODE_MANUAL;
            setServoAngle(ch, ang);
            Serial.printf("🎯 [개별 제어] 채널 %d 서보모터 -> %.1f° 이동 완료 (12비트 펄스: %d)\n", ch, ang, angleToPulse(ang));
          } else {
            Serial.println(F("⚠️ 채널(0~15)과 각도(0~180)를 올바르게 입력해주세요 (예: 0 130)"));
          }
        } else {
          // [전체 각도 제어: 숫자 1개 입력 -> 예: 150, 130, 170]
          float val = input.toFloat();
          if (val >= 0.0 && val <= 180.0) {
            currentMode = MODE_MANUAL;
            setAllServos(val);
            Serial.printf("🎯 [전체 제어] 채널 0~7 전체 모터 -> %.1f° 이동 완료 (12비트 펄스: %d)\n", val, angleToPulse(val));
          } else {
            Serial.println(F("⚠️ 유효한 각도(0~180) 또는 명령어(test, wave, sweep, home)를 입력해주세요!"));
          }
        }
      }
    }
  }

  // 2. 파도타기(Wave) 모드 시 50Hz (20ms) 주기 공간 위상차 구동
  if (currentMode == MODE_WAVE) {
    if (currentMillis - lastMotionUpdate >= 20) {
      lastMotionUpdate = currentMillis;

      wavePhase += 0.025f * waveSpeed;
      if (wavePhase > 6.28318f * 100.0f) wavePhase = 0.0f;

      // 0번, 1번, 2번 모터에 공간적 위상차(0.35 rad)를 주어 파도타기 연출
      for (uint8_t i = 0; i < 3; i++) {
        float phaseOffset = i * 0.35f;
        float targetAngle = BASE_ANGLE + sin(wavePhase - phaseOffset) * waveAmplitude;
        targetAngle = constrain(targetAngle, 130.0f, 170.0f);
        setServoAngle(i, targetAngle);
      }
    }
  }
}
