#include <Servo.h>

// ==========================================
// [아두이노 우노 핀 설정 및 제어 파라미터]
// ==========================================
const int SERVO_PIN = 9;   // 아두이노 우노 디지털 9번 핀 (PWM)

const int BASE_ANGLE = 150; // 기본 위치 (150도)
const int SWING_ANGLE = 20; // 양옆 움직임 변위 (±20도)

// 150도 기준 양옆 가동 범위 (130도 ~ 170도)
const int MAX_ANGLE = BASE_ANGLE + SWING_ANGLE; // 170도
const int MIN_ANGLE = BASE_ANGLE - SWING_ANGLE; // 130도

Servo myServo;

// [1회차] 서보모터 최고 속도 팍 튕기기 (지연 없이 직행)
void runFastCycle() {
  // 150도 -> 170도 (모터 최고 속도로 즉시 이동)
  myServo.write(MAX_ANGLE);
  delay(150);

  // 170도 -> 130도 (모터 최고 속도로 즉시 이동)
  myServo.write(MIN_ANGLE);
  delay(200);

  // 130도 -> 150도 (기본 위치 복귀)
  myServo.write(BASE_ANGLE);
  delay(300);
}

// [2회차] 서보모터 초슬로우 움직임 (1도당 90ms 지연 -> 사르르 매우 느리게)
void runSlowCycle(int stepDelayMs) {
  // 1. 150도 -> 170도
  for (int angle = BASE_ANGLE; angle <= MAX_ANGLE; angle++) {
    myServo.write(angle);
    delay(stepDelayMs);
  }
  delay(200);

  // 2. 170도 -> 130도
  for (int angle = MAX_ANGLE; angle >= MIN_ANGLE; angle--) {
    myServo.write(angle);
    delay(stepDelayMs);
  }
  delay(200);

  // 3. 130도 -> 150도
  for (int angle = MIN_ANGLE; angle <= BASE_ANGLE; angle++) {
    myServo.write(angle);
    delay(stepDelayMs);
  }
  delay(300);
}

void setup() {
  Serial.begin(9600);
  
  myServo.attach(SERVO_PIN);
  myServo.write(BASE_ANGLE);
  delay(500);

  Serial.println(F("\n==========================================="));
  Serial.println(F("Arduino UNO 서보모터 속도차 극대화 테스트!"));
  Serial.println(F("시리얼 모니터에 '1' 또는 'run'을 입력하세요."));
  Serial.println(F("===========================================\n"));
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "1" || input == "run" || input == "RUN") {
      Serial.println(F("\n>>> 테스트 동작 시작!"));

      // [1회차] 순식간에 채찍처럼 최고 속도 타격
      Serial.println(F("[1회차] ⚡ 순간 최고속도 (0.2초 만에 촥 튕김!)"));
      runFastCycle();
      
      delay(1500); // 1.5초 대기

      // [2회차] 1도당 90ms로 사르르 아주 느린 움직임 (총 7초 동안 천천히 이동)
      Serial.println(F("[2회차] 🍃 초슬로우 극강의 느린 움직임 (7초 동안 사르르...)"));
      runSlowCycle(90); // 90ms 지연 (엄청 느림)

      Serial.println(F(">>> 1회차(초고속) & 2회차(초슬로우) 동작 완료!\n"));
    }
  }
}
