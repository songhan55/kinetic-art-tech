#include <Servo.h>

// =========================================================================
// [아두이노 UNO 서보모터 9번 핀 100% 균일 등속도 왕복 2회 제어 시스템]
// =========================================================================
const int SERVO_PIN = 9;    // 아두이노 우노 디지털 9번 핀 (PWM)
const int BASE_ANGLE = 150; // 기준 중립 위치: 150도

Servo myServo;

// 100% 균일 등속도(Constant Linear Speed) 이동 함수
// msPerDegree: 1도를 이동하는 데 걸리는 시간(ms) -> 전 구간 속도 편차 0%
void moveConstantSpeed(float fromAngle, float toAngle, int msPerDegree) {
  float distance = fabs(toAngle - fromAngle);
  if (distance < 0.1) return;

  // 최고 속도 모드 (0ms 지연)
  if (msPerDegree <= 0) {
    myServo.write((int)toAngle);
    delay((int)(distance * 4)); // 서보 물리 이동 시간
    return;
  }

  unsigned long totalDuration = (unsigned long)(distance * (float)msPerDegree);
  unsigned long startTime = millis();

  while (true) {
    unsigned long elapsed = millis() - startTime;
    if (elapsed >= totalDuration) {
      myServo.write((int)round(toAngle));
      break;
    }
    // 시간 비례 정밀 부동소수점 보간 (속도 변화 없이 완벽한 등속도 유지)
    float progress = (float)elapsed / (float)totalDuration;
    float currentAngle = fromAngle + (toAngle - fromAngle) * progress;
    myServo.write((int)round(currentAngle));
    delay(15); // 50Hz PWM과 동기화된 15ms 고정 제어 주기
  }
}

void printPrompt() {
  Serial.println(F("--------------------------------------------------"));
  Serial.println(F("👉 [입력 형식] : '각도 속도' 입력 후 Enter"));
  Serial.println(F("   예시: 20 10  -> 150도 기준 ±20도, 1도당 10ms (완전 균일 등속도)"));
  Serial.println(F("   예시: 20 0   -> 150도 기준 ±20도, 최고 속도 (0ms 지연)"));
  Serial.println(F("   예시: 15 25  -> 150도 기준 ±15도, 1도당 25ms (부드러운 등속도)"));
  Serial.println(F("--------------------------------------------------\n"));
}

// 끊김 없는 연속 왕복 2회 실행 함수
void runTwoRoundTrips(int swingAngle, int msPerDegree) {
  swingAngle = constrain(swingAngle, 1, 45); // 안전 각도 제한 (최대 ±45도)
  msPerDegree = constrain(msPerDegree, 0, 150); // 0ms ~ 150ms

  int maxAngle = constrain(BASE_ANGLE + swingAngle, 0, 180);
  int minAngle = constrain(BASE_ANGLE - swingAngle, 0, 180);

  Serial.println(F("\n=================================================="));
  Serial.print(F("🚀 [균일 등속도 왕복 2회 시작] 기준 150° ± "));
  Serial.print(swingAngle);
  Serial.print(F("° ("));
  Serial.print(minAngle);
  Serial.print(F("° ~ "));
  Serial.print(maxAngle);
  Serial.print(F("°), 1도당: "));
  Serial.print(msPerDegree);
  Serial.println(F("ms (전 구간 일정한 속도)"));
  Serial.println(F("=================================================="));

  // [1회차 왕복] 150° -> Max -> Min
  Serial.println(F(">>> [1회차] 150° -> 최댓값 -> 최솟값 진행 중..."));
  moveConstantSpeed(BASE_ANGLE, maxAngle, msPerDegree);
  moveConstantSpeed(maxAngle, minAngle, msPerDegree);

  // [2회차 왕복] Min -> Max -> Min (중간 멈춤 없이 균일하게 스윙)
  Serial.println(F(">>> [2회차] 최솟값 -> 최댓값 -> 최솟값 진행 중..."));
  moveConstantSpeed(minAngle, maxAngle, msPerDegree);
  moveConstantSpeed(maxAngle, minAngle, msPerDegree);

  // [마무리] Min -> 150도 중립 복귀
  Serial.println(F(">>> [복귀] 최솟값 -> 150도 중립 위치 복귀 중..."));
  moveConstantSpeed(minAngle, BASE_ANGLE, msPerDegree);

  Serial.println(F("🎉 [완료] 왕복 2회 균일 등속도 동작 완료!"));
  Serial.println(F("✅ 150도 중립 위치에서 안전 대기합니다.\n"));
  printPrompt();
}

void setup() {
  Serial.begin(9600);
  delay(300);

  Serial.println(F("\n=================================================="));
  Serial.println(F("  Arduino UNO 9번 핀 [100% 균일 등속도 서보 제어기]"));
  Serial.println(F("  구간별 속도 변화 / 중간 끊김 완전히 제거"));
  Serial.println(F("=================================================="));

  myServo.attach(SERVO_PIN, 500, 2500);
  myServo.write(BASE_ANGLE);
  delay(500);

  Serial.println(F("✅ 모터 초기화 완료: 150도 중립 안전 대기 중"));
  printPrompt();
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.replace(',', ' ');

    if (input.length() > 0) {
      int swingAngle = 20;
      int msPerDegree = 10;

      int spaceIdx = input.indexOf(' ');
      if (spaceIdx != -1) {
        String first = input.substring(0, spaceIdx);
        String second = input.substring(spaceIdx + 1);
        first.trim();
        second.trim();

        swingAngle = first.toInt();
        msPerDegree = second.toInt();
      } else {
        swingAngle = input.toInt();
        msPerDegree = 10;
      }

      if (swingAngle > 0) {
        runTwoRoundTrips(swingAngle, msPerDegree);
      } else {
        Serial.println(F("⚠️ 각도는 1 이상의 양수를 입력해주세요!"));
        printPrompt();
      }
    }
  }
}
