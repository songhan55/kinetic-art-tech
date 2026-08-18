#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 21
#define SCL_PIN 22
#define PCA9685_ADDR 0x40

// PCA9685 레지스터
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01
#define PCA9685_PRESCALE 0xFE
#define LED0_ON_L 0x06
#define LED0_ON_H 0x07
#define LED0_OFF_L 0x08
#define LED0_OFF_H 0x09
#define ALLLED_ON_L 0xFA
#define ALLLED_ON_H 0xFB
#define ALLLED_OFF_L 0xFC
#define ALLLED_OFF_H 0xFD

void writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)PCA9685_ADDR, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0xFF;
}

// 50Hz에서 12비트 펄스 직접 출력 (모든 채널에 토템폴 푸시풀 출력)
void setAllPwmDirect(uint16_t pulse) {
  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(ALLLED_ON_L);
  Wire.write(0x00); // ON_L
  Wire.write(0x00); // ON_H
  Wire.write((uint8_t)(pulse & 0xFF)); // OFF_L
  Wire.write((uint8_t)(pulse >> 8));   // OFF_H
  Wire.endTransmission();
}

void setChannelPwmDirect(uint8_t ch, uint16_t pulse) {
  uint8_t reg = LED0_ON_L + 4 * ch;
  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(reg);
  Wire.write(0x00); // ON_L
  Wire.write(0x00); // ON_H
  Wire.write((uint8_t)(pulse & 0xFF)); // OFF_L
  Wire.write((uint8_t)(pulse >> 8));   // OFF_H
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  delay(300);

  Serial.println(F("\n=================================================="));
  Serial.println(F("  ⚡ [PCA9685 하드웨어 레지스터 직결 서보 엔진]"));
  Serial.println(F("  라이브러리 종속성 완전 제거 / 순수 하드웨어 I2C 제어"));
  Serial.println(F("=================================================="));

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // 1. PCA9685 완전 하드웨어 리셋
  writeRegister(PCA9685_MODE1, 0x00); // 일반 모드 (SLEEP 해제)
  delay(5);

  // 2. 50Hz 프리스케일러 설정 (25MHz / (4096 * 50Hz) - 1 = 121 = 0x79)
  writeRegister(PCA9685_MODE1, 0x10); // SLEEP = 1 (프리스케일러 변경 시 필수)
  writeRegister(PCA9685_PRESCALE, 121); // 50Hz 설정 (0x79)
  writeRegister(PCA9685_MODE1, 0x00); // SLEEP = 0 (오실레이터 가동)
  delay(5);
  // Auto-Increment 활성화 (0x20)
  writeRegister(PCA9685_MODE1, 0x20);

  // 3. MODE2: 토템폴 푸시풀 출력 설정 (0x04: OUTDRV=1 -> 서보 구동 필수!)
  writeRegister(PCA9685_MODE2, 0x04);

  // 레지스터 상태 검증 출력
  uint8_t m1 = readRegister(PCA9685_MODE1);
  uint8_t m2 = readRegister(PCA9685_MODE2);
  uint8_t ps = readRegister(PCA9685_PRESCALE);
  Serial.printf("📊 [PCA9685 레지스터 상태] MODE1: 0x%02X | MODE2: 0x%02X | PRESCALE: %d (50Hz)\n", m1, m2, ps);

  if (m1 == 0xFF) {
    Serial.println(F("❌ [I2C 응답 없음] PCA9685가 응답하지 않습니다. 배선을 확인해주세요."));
  } else {
    Serial.println(F("🎉 [하드웨어 초기화 완료] 16개 모든 채널에 150도(444 펄스) 주입 중..."));
    setAllPwmDirect(444);
  }
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();
    input.replace(',', ' ');
    input.replace('\r', ' ');
    input.trim();

    if (input.length() > 0) {
      Serial.printf("👉 [명령 수신] \"%s\"\n", input.c_str());

      if (input == "test" || input == "t") {
        Serial.println(F("🔍 [자가진단 펄스 출력] 채널 0, 1, 2 순차 테스트"));
        for (int ch = 0; ch < 3; ch++) {
          Serial.printf(">>> [채널 %d] 130도(398) -> 170도(489) -> 150도(444)\n", ch);
          setChannelPwmDirect(ch, 398);
          delay(500);
          setChannelPwmDirect(ch, 489);
          delay(500);
          setChannelPwmDirect(ch, 444);
          delay(300);
        }
        Serial.println(F("✅ [테스트 펄스 송신 완료]"));
      } else if (input == "wave" || input == "w") {
        Serial.println(F("🌊 [연속 파도타기 시작] 10초간 실행 (아무 키나 누르면 정지)..."));
        unsigned long start = millis();
        float phase = 0.0;
        while (millis() - start < 10000) {
          if (Serial.available() > 0) break;
          phase += 0.08;
          for (int ch = 0; ch < 3; ch++) {
            float ang = 150.0 + sin(phase - ch * 0.45) * 16.0;
            // 102 (0도) ~ 512 (180도)
            int pulse = (int)(102.0 + (ang / 180.0) * 410.0);
            setChannelPwmDirect(ch, pulse);
          }
          delay(20);
        }
        setAllPwmDirect(444);
        Serial.println(F("✅ [파도 완료] 150도 홈 포지션 복귀"));
      } else {
        float val = input.toFloat();
        if (val >= 0 && val <= 180) {
          int pulse = (int)(102.0 + (val / 180.0) * 410.0);
          setAllPwmDirect(pulse);
          Serial.printf("🎯 [16개 전 채널 펄스 직접 출력] 각도: %.1f° -> 펄스: %d\n", val, pulse);
        } else {
          Serial.println(F("⚠️ 각도(0~180) 또는 'test', 'wave'를 입력해주세요"));
        }
      }
    }
  }
}
