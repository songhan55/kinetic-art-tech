#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 브라운아웃(모터 전압 강하 리셋) 방지 헤더
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// =========================================================================
// [1. PCA9685 16채널 I2C 서보 드라이버 설정]
// =========================================================================
// ESP32 기본 I2C 핀: SDA = GPIO 21, SCL = GPIO 22
#define SDA_PIN 21
#define SCL_PIN 22
#define PCA9685_ADDR 0x40
#define NUM_SERVOS 3

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);
bool pca9685Found = false;

const float BASE_ANGLE = 150.0; // 기준 중립 위치: 150도
float currentAngles[NUM_SERVOS] = {150.0, 150.0, 150.0};

// 동작 모드 상태
enum Mode {
  MODE_MANUAL,    // 직접 각도 지정 고정 모드
  MODE_WAVE,      // 3개 모터 연속 파도타기 모드
  MODE_SWEEP      // 130도 ~ 170도 왕복 스위프 모드
};

Mode currentMode = MODE_MANUAL;
float wavePhase = 0.0;
float waveSpeed = 2.5;
float waveAmplitude = 15.0; // ±15도 스윙 (135° ~ 165°)
unsigned long lastMotionUpdate = 0;

// 각도(0~180도) -> 마이크로초(500us ~ 2500us) 정밀 변환
inline int angleToMicros(float angle) {
  angle = constrain(angle, 0.0f, 180.0f);
  return (int)(500.0f + (angle / 180.0f) * 2000.0f);
}

void setServoAngle(uint8_t channel, float angle) {
  if (channel >= NUM_SERVOS) return;
  angle = constrain(angle, 0.0f, 180.0f);
  currentAngles[channel] = angle;
  if (pca9685Found) {
    pwm.writeMicroseconds(channel, angleToMicros(angle));
  }
}

void setAllServos(float angle) {
  for (uint8_t i = 0; i < NUM_SERVOS; i++) {
    setServoAngle(i, angle);
  }
}

void printMenu() {
  Serial.println(F("\n=================================================="));
  Serial.println(F("  🎮 [ESP32 + PCA9685 3채널 서보 터미널 테스트기]"));
  Serial.println(F("=================================================="));
  Serial.println(F("👉 [전체 각도 제어] 숫자만 입력 후 Enter"));
  Serial.println(F("   예: 150    -> 3개 모터 전부 150도(중립 홈)로 즉시 회전"));
  Serial.println(F("   예: 130    -> 3개 모터 전부 130도로 즉시 회전"));
  Serial.println(F("   예: 170    -> 3개 모터 전부 170도로 즉시 회전"));
  Serial.println(F("👉 [개별 채널 제어] '채널 각도'"));
  Serial.println(F("   예: 0 130  -> 0번 모터만 130도로 회전"));
  Serial.println(F("   예: 1 150  -> 1번 모터만 150도로 회전"));
  Serial.println(F("   예: 2 170  -> 2번 모터만 170도로 회전"));
  Serial.println(F("👉 [동적 연출 모드]"));
  Serial.println(F("   예: wave   -> 3개 모터가 시차(위상차)를 두고 출렁이는 연속 파도"));
  Serial.println(F("   예: sweep  -> 130도 ~ 170도 사이를 부드럽게 2회 왕복 스위프"));
  Serial.println(F("   예: home   -> 3개 모터 150도 중립 정렬 및 정지 대기"));
  Serial.println(F("==================================================\n"));
}

// 등속도 왕복 2회 스위프 함수
void runSweepTest(float minAng, float maxAng, int msDelay) {
  Serial.println(F("🚀 [스위프 시작] 130° ~ 170° 왕복 2회 진행 중..."));
  for (int round = 1; round <= 2; round++) {
    // 150 -> Max
    for (float a = BASE_ANGLE; a <= maxAng; a += 1.0) {
      setAllServos(a);
      delay(msDelay);
    }
    // Max -> Min
    for (float a = maxAng; a >= minAng; a -= 1.0) {
      setAllServos(a);
      delay(msDelay);
    }
    // Min -> 150
    for (float a = minAng; a <= BASE_ANGLE; a += 1.0) {
      setAllServos(a);
      delay(msDelay);
    }
  }
  Serial.println(F("✅ [스위프 완료] 150도 홈 포지션 복귀 완료!"));
}

void setup() {
  // 브라운아웃 디텍터 비활성화 (모터 피크 전류 시 ESP32 재부팅 방지)
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(300);

  Serial.println(F("\n[초기화] ESP32 I2C 버스 시작 (SDA: GPIO 21, SCL: GPIO 22)..."));
  Wire.begin(SDA_PIN, SCL_PIN);

  if (pwm.begin()) {
    pca9685Found = true;
    pwm.setPWMFreq(50); // 50Hz RC 서보 표준 주파수
    Serial.println(F("🎉 [PCA9685 감지 성공] 16채널 I2C 드라이버가 정상 연결되었습니다!"));
    // 3개 모터 150도 중립 정렬
    setAllServos(BASE_ANGLE);
  } else {
    Serial.println(F("❌ [PCA9685 감지 실패] I2C 주소(0x40) 또는 SDA/SCL 배선을 확인해주세요."));
  }

  printMenu();
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. 시리얼 터미널 사용자 입력 처리
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();
    input.replace(',', ' ');

    if (input.length() > 0) {
      if (input == "home" || input == "h" || input == "stop") {
        currentMode = MODE_MANUAL;
        setAllServos(BASE_ANGLE);
        Serial.println(F("🎯 [홈 포지션] 3개 서보모터 전체 150도 정렬 및 정지 대기 완료"));
      } else if (input == "wave" || input == "w") {
        currentMode = MODE_WAVE;
        wavePhase = 0.0;
        Serial.println(F("🌊 [파도 모드 시작] 3개 모터가 위상차를 두고 유기적으로 출렁입니다! (정지는 'home' 입력)"));
      } else if (input == "sweep" || input == "s") {
        currentMode = MODE_MANUAL;
        runSweepTest(130.0, 170.0, 15);
      } else {
        int spaceIdx = input.indexOf(' ');
        if (spaceIdx != -1) {
          // [개별 채널 제어: '채널 각도' -> 예: 0 130]
          String chStr = input.substring(0, spaceIdx);
          String angStr = input.substring(spaceIdx + 1);
          chStr.trim();
          angStr.trim();

          int ch = chStr.toInt();
          float ang = angStr.toFloat();
          if (ch >= 0 && ch < NUM_SERVOS && ang >= 0.0 && ang <= 180.0) {
            currentMode = MODE_MANUAL;
            setServoAngle(ch, ang);
            Serial.printf("🎯 [개별 제어] 채널 %d 서보모터 -> %.1f° 이동 완료\n", ch, ang);
          } else {
            Serial.println(F("⚠️ 채널(0~2)과 각도(0~180)를 올바르게 입력해주세요 (예: 0 130)"));
          }
        } else {
          // [전체 각도 제어: 숫자 1개 입력 -> 예: 150, 130, 170]
          float val = input.toFloat();
          if (val >= 0.0 && val <= 180.0) {
            currentMode = MODE_MANUAL;
            setAllServos(val);
            Serial.printf("🎯 [전체 제어] 3개 모터 전체 -> %.1f° 이동 완료\n", val);
          } else {
            Serial.println(F("⚠️ 유효한 각도(0~180) 또는 명령어(wave, sweep, home)를 입력해주세요!"));
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

      // 3개 모터에 공간적 위상차(0.35 rad)를 주어 파도타기 연출
      for (uint8_t i = 0; i < NUM_SERVOS; i++) {
        float phaseOffset = i * 0.35f;
        float targetAngle = BASE_ANGLE + sin(wavePhase - phaseOffset) * waveAmplitude;
        targetAngle = constrain(targetAngle, 130.0f, 170.0f);
        setServoAngle(i, targetAngle);
      }
    }
  }
}
