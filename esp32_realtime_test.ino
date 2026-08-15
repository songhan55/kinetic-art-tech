/*
 * ESP32 Real-Time Trend Data Fetch & Serial Monitor Test
 * 
 * 1. ESP32 Wi-Fi 접속
 * 2. 실시간 트렌드 데이터 수신 (10초 주기)
 * 3. 시리얼 모니터(115200 baud)에 시간, 실시간 점수(0~100), 모터 진폭 각도 출력
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ★ 사용하시는 와이파이 이름과 비밀번호로 변경해주세요 ★
const char* ssid = "MIRR";
const char* password = "mirr3411";

// Data Refresh Interval (10초)
const unsigned long FETCH_INTERVAL = 10000;
unsigned long lastFetchTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n==================================================");
  Serial.println("  ESP32 Real-Time Trend Data Serial Test (10s)");
  Serial.println("==================================================");
  
  WiFi.begin(ssid, password);
  Serial.print("[Wi-Fi] 연결 중...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[Wi-Fi] 성공적으로 연결되었습니다! IP: " + WiFi.localIP().toString());
  
  fetchRealTimeTrend();
}

void loop() {
  if (millis() - lastFetchTime >= FETCH_INTERVAL) {
    lastFetchTime = millis();
    fetchRealTimeTrend();
  }
}

void fetchRealTimeTrend() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = "https://api.gdeltproject.org/api/v2/doc/doc?query=war&mode=timelinevol&format=json";

  http.begin(client, url);
  http.addHeader("User-Agent", "ESP32-KineticArt");

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonArray dataArr = doc["timeline"][0]["data"].as<JsonArray>();
      JsonObject latest = dataArr[dataArr.size() - 1];
      
      const char* dt = latest["datetime"] | "N/A";
      float rawVal = latest["value"] | 0.0;

      // 20.0%를 만점(100점) 기준선으로 설정
      float score100 = constrain((rawVal / 20.0) * 100.0, 0.0, 100.0);
      float mappedAngle = 5.0 + (score100 / 100.0) * 15.0;

      Serial.println("\n--------------------------------------------------");
      Serial.printf("[수신 시간]: %s\n", dt);
      Serial.printf("[원본 데이터]: %.4f%%\n", rawVal);
      Serial.printf("[실시간 트렌드 점수]: %.1f / 100\n", score100);
      Serial.printf("[모터 진폭 각도]: +/-%.2f deg (Home 150.0 deg)\n", mappedAngle);
      Serial.println("--------------------------------------------------");
    } else {
      Serial.print("[JSON 오류]: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.printf("[HTTP 오류]: Code %d\n", httpCode);
  }

  http.end();
}
