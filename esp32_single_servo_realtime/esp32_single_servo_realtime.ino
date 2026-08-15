#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// =========================================================================
// [1. 핀 배선 및 모터 파라미터]
// =========================================================================
const int SERVO_PIN = 18;       // 서보 신호선: GPIO 18
const float BASE_ANGLE = 150.0; // 기본 중립 위치: 150도

Servo singleServo;
WebServer server(80);

// =========================================================================
// [2. Wi-Fi 정보] (멀티 노드 자동 식별 시스템)
// =========================================================================
const char* ssid = "MIRR";
const char* password = "mirr3411";

String nodeName = "node1";
String macAddressStr = "";
String localIPStr = "Connecting...";

// 원격 제어 & 모니터링 공유 변수 (멀티스레드 안전)
volatile bool isPaused = false;            // 긴급 정지 플래그
volatile float sharedScore = 42.0;         // 실시간 소셜 지수 (0 ~ 100)
volatile bool isWifiConnected = false;
String lastPostSnippet = "System Initialized";
String lastPostTime = "N/A";

// 모터 모션 변수
float smoothedScore = 42.0;
float currentAmplitude = 11.3;
float currentSpeed = 2.9;
float wavePhase = 0.0;
unsigned long lastMotionUpdate = 0;
unsigned long lastScoreUpdate = 0;

// =========================================================================
// [3. MAC 주소 기반 노드 자동 식별]
// =========================================================================
void identifyNode() {
  macAddressStr = WiFi.macAddress();
  macAddressStr.toLowerCase();

  if (macAddressStr == "b8:d6:1a:65:ea:60") {
    nodeName = "node1";
  } else if (macAddressStr == "8c:94:df:6d:8b:c4") {
    nodeName = "node2";
  } else {
    // 기타 보드는 MAC 끝자리 4자리로 고유 이름 부여
    nodeName = "node-" + macAddressStr.substring(12, 14) + macAddressStr.substring(15, 17);
  }

  Serial.println("==================================================");
  Serial.printf("  [노드 식별] MAC: %s -> Node ID: %s\n", macAddressStr.c_str(), nodeName.c_str());
  Serial.printf("  [접속 주소] http://%s.local\n", nodeName.c_str());
  Serial.println("==================================================");
}

// =========================================================================
// [4. 웹서버 HTTP API 핸들러 (CORS 허용)]
// =========================================================================
void setCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

void handleStatus() {
  setCORS();
  DynamicJsonDocument doc(512);
  doc["nodeId"] = nodeName;
  doc["mac"] = macAddressStr;
  doc["paused"] = isPaused;
  doc["score"] = smoothedScore;
  doc["amp"] = currentAmplitude;
  doc["post"] = lastPostSnippet;
  doc["time"] = lastPostTime;
  doc["ip"] = localIPStr;

  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}

void handlePause() {
  setCORS();
  isPaused = true;
  singleServo.write(BASE_ANGLE); // 즉시 150도 중립 안전 각도로 파킹!
  Serial.printf("\n[🛑 긴급 정지] %s -> 150도 중립 안전 파킹 완료!\n", nodeName.c_str());
  handleStatus();
}

void handleResume() {
  setCORS();
  isPaused = false;
  Serial.printf("\n[▶️ 실시간 재가동] %s -> 실시간 소셜 파도 모션 재개!\n", nodeName.c_str());
  handleStatus();
}

void handleSyncPhase() {
  setCORS();
  wavePhase = 0.0; // 파도 위상 0으로 즉시 초기화 (동시 동기화)
  Serial.printf("\n[🌊 파도 동기화] %s -> wavePhase = 0 초기화 완료!\n", nodeName.c_str());
  handleStatus();
}

void handleSweep() {
  setCORS();
  Serial.printf("\n[🎯 자가진단] %s -> 진단 스윙 테스트 실행\n", nodeName.c_str());
  singleServo.write(165);
  delay(300);
  singleServo.write(135);
  delay(300);
  singleServo.write(BASE_ANGLE);
  handleStatus();
}

void handleReboot() {
  setCORS();
  server.send(200, "application/json", "{\"status\":\"rebooting\"}");
  delay(500);
  ESP.restart();
}

// =========================================================================
// [Core 0 백그라운드 태스크: Wi-Fi 통신, 오픈 소셜 수신 및 웹서버 구동]
// =========================================================================
void dataFetchTask(void *pvParameters) {
  Serial.printf("[Core 0] Wi-Fi '%s' 백그라운드 연결 시작...\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long lastFetchTime = 0;

  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!isWifiConnected) {
        isWifiConnected = true;
        localIPStr = WiFi.localIP().toString();

        // mDNS 등록 (http://node1.local / http://node2.local)
        if (MDNS.begin(nodeName.c_str())) {
          Serial.printf("[mDNS] http://%s.local 등록 완료!\n", nodeName.c_str());
        }
        MDNS.addService("http", "tcp", 80);

        // 웹서버 라우트 등록
        server.on("/api/status", HTTP_GET, handleStatus);
        server.on("/api/pause", HTTP_GET, handlePause);
        server.on("/api/resume", HTTP_GET, handleResume);
        server.on("/api/sync_phase", HTTP_GET, handleSyncPhase);
        server.on("/api/sweep", HTTP_GET, handleSweep);
        server.on("/api/reboot", HTTP_GET, handleReboot);
        server.begin();

        Serial.printf("  🎉 [%s 웹서버 가동] http://%s 또는 http://%s.local\n", 
                      nodeName.c_str(), localIPStr.c_str(), nodeName.c_str());
      }

      // 웹서버 클라이언트 요청 처리
      server.handleClient();

      // 5초마다 오픈 소셜 타임라인 수신 (정지 상태가 아닐 때만)
      if (millis() - lastFetchTime >= 5000) {
        lastFetchTime = millis();

        if (!isPaused) {
          WiFiClientSecure client;
          client.setInsecure();

          HTTPClient http;
          String url = "https://mastodon.social/api/v1/timelines/tag/war?limit=5";

          http.begin(client, url);
          http.setTimeout(4000);
          http.addHeader("User-Agent", "KineticArt-LiveSocialBot/1.0");

          int httpCode = http.GET();
          if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            DynamicJsonDocument doc(8192);
            DeserializationError error = deserializeJson(doc, payload);

            if (!error && doc.is<JsonArray>()) {
              JsonArray posts = doc.as<JsonArray>();
              if (posts.size() > 0) {
                JsonObject latest = posts[0];
                lastPostTime = latest["created_at"] | "Just now";
                const char* contentRaw = latest["content"] | "";

                String cleanText = "";
                bool inTag = false;
                for (int i = 0; contentRaw[i] != '\0' && i < 120; i++) {
                  if (contentRaw[i] == '<') inTag = true;
                  else if (contentRaw[i] == '>') inTag = false;
                  else if (!inTag) cleanText += contentRaw[i];
                }
                cleanText.trim();
                if (cleanText.length() > 60) cleanText = cleanText.substring(0, 60) + "...";
                lastPostSnippet = cleanText;

                float postCountScore = 35.0f + (posts.size() * 6.5f);
                sharedScore = constrain(postCountScore, 10.0f, 95.0f);
              }
            }
          }
          http.end();
        }
      }
    } else {
      isWifiConnected = false;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS); // 웹서버 반응성을 위해 10ms 슬립
  }
}

// =========================================================================
// [Core 1 메인 태스크: 초당 50회 끊김 없는 실시간 모터 구동]
// =========================================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  // 1. MAC 주소 읽고 노드 이름 결정 (Node 1 또는 Node 2)
  identifyNode();

  // 2. 서보 핀 설정
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  singleServo.setPeriodHertz(50);
  singleServo.attach(SERVO_PIN, 500, 2500);

  // 3. 부팅 진단 스윙
  singleServo.write(165);
  delay(250);
  singleServo.write(135);
  delay(250);
  singleServo.write(BASE_ANGLE);
  delay(250);

  // 4. Core 0 통신 & 웹서버 백그라운드 태스크 시작
  xTaskCreatePinnedToCore(
    dataFetchTask,
    "DataFetchTask",
    12288, // 웹서버 포함 12KB 스택
    NULL,
    1,
    NULL,
    0
  );
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. 0.5초마다 점수 스무딩
  if (currentMillis - lastScoreUpdate >= 500) {
    lastScoreUpdate = currentMillis;

    if (isPaused) {
      currentAmplitude = 0.0;
    } else {
      float currentTarget = sharedScore;
      smoothedScore = smoothedScore * 0.85f + currentTarget * 0.15f;
      currentAmplitude = 5.0f + (smoothedScore / 100.0f) * 15.0f;
      currentSpeed = 1.5f + (smoothedScore / 100.0f) * 3.5f;
    }
  }

  // 2. 초당 50회(20ms) 연속 서보 구동
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
