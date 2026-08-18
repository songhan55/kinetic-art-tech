#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 브라운아웃(전압 강하 리셋) 방지 헤더
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// =========================================================================
// [1. 핀 배선 및 모터/센서 파라미터]
// =========================================================================
const int SERVO_PIN = 18;       // 서보 단독 신호선: GPIO 18 (직결용)
const float BASE_ANGLE = 150.0; // 기본 중립 위치: 150도

// I2C 및 PCA9685 16채널 PWM 드라이버 (SDA: GPIO 21, SCL: GPIO 22)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
bool pca9685Found = false;

// HC-SR04 초음파 거리 센서 핀 배선
const int TRIG_PIN = 5;         // 초음파 트리거 펄스 출력 (GPIO 5)
const int ECHO_PIN = 19;        // 반사 에코 수신 입력 (GPIO 19)

Servo singleServo;
WebServer server(80);

// 각도(0~180도) -> 마이크로초(500us ~ 2500us) 정밀 변환 함수
inline int angleToMicros(float angle) {
  angle = constrain(angle, 0.0f, 180.0f);
  return (int)(500.0f + (angle / 180.0f) * 2000.0f);
}

// =========================================================================
// [2. Wi-Fi 정보 & ESP-NOW 통신 패킷 구조체]
// =========================================================================
const char* ssid = "MIRR";
const char* password = "mirr3411";

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
  float score;        // 실시간 국제정세 긴장도 (0 ~ 100)
  float wavePhase;    // 파도 위상 (0.0 ~ 6.28)
  float dampFactor;   // 초음파 거리 감쇄 계수 K (0.15 ~ 1.00)
  float viewerDist;   // 관람객 실측 거리 (m)
  bool isPaused;      // 긴급 정지 여부
  uint8_t cmd;        // 0: 일반, 1: 자가진단 스윙, 2: 재부팅
} struct_message;

struct_message txData;

// 마스터 상태 변수 (기본값: 실시간 국제정세 데이터 파도 모드 가동)
volatile bool isPaused = false;
volatile bool isManualAngleMode = false;
volatile float manualAngle = 150.0;
volatile float sharedScore = 31.5;
volatile bool isWifiConnected = false;
String lastPostSnippet = "System Initialized";
String lastPostTime = "N/A";
String localIPStr = "Connecting...";

// 초음파 거리 및 감쇄 변수
volatile float rawDistanceMeters = 3.0;
volatile float filteredDistanceMeters = 3.0;
volatile float currentDampFactor = 1.0;
unsigned long lastUltrasonicPing = 0;
unsigned long lastDebugPrint = 0;

float smoothedScore = 31.5;
float currentAmplitude = 8.5;
float currentSpeed = 2.5;
float wavePhase = 0.0;
unsigned long lastMotionUpdate = 0;
unsigned long lastScoreUpdate = 0;
unsigned long lastEspNowSend = 0;

// =========================================================================
// [3. ESP32 내장 통합 대시보드 웹페이지]
// =========================================================================
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>키네틱 아트 [ESP-NOW 무선 직결] 관제 센터</title>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700;800;900&family=JetBrains+Mono:wght@500;700;800&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #0c1017;
      --card-bg: #161c28;
      --card-inner: #1e2638;
      --accent-green: #10b981;
      --accent-gold: #cba258;
      --accent-red: #ef4444;
      --accent-blue: #0285FF;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --border: rgba(255, 255, 255, 0.08);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Inter', sans-serif; }
    body { background-color: var(--bg); color: var(--text); min-height: 100vh; display: flex; flex-direction: column; align-items: center; padding: 32px 16px; }
    .container { width: 100%; max-width: 680px; display: flex; flex-direction: column; gap: 18px; }
    .card { background: var(--card-bg); border-radius: 18px; padding: 22px 24px; border: 1px solid var(--border); box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
    .header-card { display: flex; justify-content: space-between; align-items: center; }
    .badge { padding: 8px 16px; border-radius: 50px; font-size: 13.5px; font-weight: 800; display: inline-flex; align-items: center; gap: 8px; }
    .badge-run { background: rgba(16, 185, 129, 0.15); color: var(--accent-green); border: 1px solid rgba(16, 185, 129, 0.3); }
    .badge-pause { background: rgba(239, 68, 68, 0.15); color: var(--accent-red); border: 1px solid rgba(239, 68, 68, 0.3); animation: pulse-red 1.5s infinite; }
    @keyframes pulse-red { 0%, 100% { transform: scale(1); } 50% { transform: scale(1.02); } }
    
    .global-controls { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }
    .btn-global { padding: 20px 16px; border-radius: 14px; border: none; cursor: pointer; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 6px; color: white; font-weight: 800; font-size: 16px; transition: all 0.2s ease; }
    .btn-global:active { transform: scale(0.96); }
    .btn-stop { background: linear-gradient(135deg, #ef4444, #b91c1c); box-shadow: 0 6px 20px rgba(239,68,68,0.35); }
    .btn-resume { background: linear-gradient(135deg, #10b981, #047857); box-shadow: 0 6px 20px rgba(16,185,129,0.35); }
    
    .btn-sync { width: 100%; padding: 14px; border-radius: 12px; background: linear-gradient(135deg, #0285FF, #0369a1); border: none; color: white; font-size: 15px; font-weight: 800; cursor: pointer; margin-top: 10px; }
    .btn-sub-row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 10px; }
    .btn-sub { padding: 12px; border-radius: 10px; background: var(--card-inner); border: 1px solid var(--border); color: var(--text); font-size: 13px; font-weight: 700; cursor: pointer; }
    
    .stat-row { display: flex; justify-content: space-between; align-items: center; font-size: 14px; border-bottom: 1px solid rgba(255,255,255,0.06); padding-bottom: 10px; margin-bottom: 10px; }
    .stat-val { font-family: 'JetBrains Mono', monospace; font-weight: 700; color: var(--accent-gold); }
    .post-box { background: var(--card-inner); border-radius: 10px; padding: 12px 14px; font-size: 13px; color: var(--text-muted); border-left: 3px solid var(--accent-blue); line-height: 1.5; margin-top: 8px; }
  </style>
</head>
<body>
  <div class="container">
    <div class="card header-card">
      <div>
        <h1 style="font-size: 19px; font-weight: 900;">🕹️ 키네틱 아트 [ESP-NOW 무선 직결] 관제 센터</h1>
        <p style="font-size: 12.5px; color: var(--text-muted); margin-top: 3px;">마스터(C to C) ➡️ 슬레이브(A to C) 0.001초 MAC 무선 직결</p>
      </div>
      <div id="status-badge" class="badge badge-run">🟢 전체 정상 가동 중</div>
    </div>

    <!-- Master Action Controls -->
    <div class="card">
      <div style="font-size: 13.5px; font-weight: 800; color: var(--accent-gold); margin-bottom: 12px;">
        ⚡ 마스터 ➡️ 전체 슬레이브 1ms 무선 일괄 명령
      </div>
      <div class="global-controls">
        <button class="btn-global btn-stop" onclick="sendCmd('/api/pause')">
          <span style="font-size: 26px;">🛑</span>
          <span>전체 긴급 정지</span>
          <span style="font-size: 11px; opacity: 0.85; font-weight: 500;">모든 모터 150° 안전 고정</span>
        </button>
        <button class="btn-global btn-resume" onclick="sendCmd('/api/resume')">
          <span style="font-size: 26px;">▶️</span>
          <span>전체 실시간 가동</span>
          <span style="font-size: 11px; opacity: 0.85; font-weight: 500;">소셜 파도 실시간 복귀</span>
        </button>
      </div>
      <button class="btn-sync" onclick="sendCmd('/api/sync_phase')">🌊 전체 움직임 동기화 (0.001초 위상 칼일치)</button>
      <div class="btn-sub-row">
        <button class="btn-sub" onclick="sendCmd('/api/sweep')">🎯 전체 모터 자가진단 스윙</button>
        <button class="btn-sub" onclick="if(confirm('전체 보드를 원격 재부팅하시겠습니까?')) sendCmd('/api/reboot')">🔄 전체 보드 원격 재부팅</button>
      </div>
    </div>

    <!-- Telemetry Status -->
    <div class="card">
      <div style="font-size: 14px; font-weight: 800; margin-bottom: 14px;">📊 실시간 텔레메트리 상태</div>
      <div class="stat-row">
        <span style="color: var(--text-muted);">전체 모터 구동 상태</span>
        <span id="txt-state" class="stat-val" style="color: var(--accent-green);">RUNNING</span>
      </div>
      <div class="stat-row">
        <span style="color: var(--text-muted);">실시간 국제정세 긴장도</span>
        <span id="txt-score" class="stat-val">- 점</span>
      </div>
      <div class="stat-row">
        <span style="color: var(--text-muted);">초음파 센서 실측 거리</span>
        <span id="txt-dist" class="stat-val" style="color: var(--accent-gold);">- m</span>
      </div>
      <div class="stat-row">
        <span style="color: var(--text-muted);">관람객 감쇄 계수 (K)</span>
        <span id="txt-damp" class="stat-val" style="color: var(--accent-green);">-</span>
      </div>
      <div class="stat-row">
        <span style="color: var(--text-muted);">전체 모터 스윙 진폭</span>
        <span id="txt-amp" class="stat-val">-</span>
      </div>
      <div>
        <div style="font-size: 12.5px; color: var(--text-muted);">최신 포착된 실시간 전쟁 소셜 글:</div>
        <div id="txt-post" class="post-box">수신 대기 중...</div>
      </div>
    </div>
  </div>

  <script>
    async function sendCmd(endpoint) {
      try {
        const res = await fetch(endpoint);
        const data = await res.json();
        updateUI(data);
      } catch (err) {
        console.warn("명령 전송 실패:", err);
      }
    }

    function updateUI(data) {
      const badge = document.getElementById('status-badge');
      const stateTxt = document.getElementById('txt-state');
      if (data.paused) {
        badge.className = "badge badge-pause";
        badge.innerHTML = "🛑 전체 긴급 정지됨 (150° 대기)";
        stateTxt.innerText = "PAUSED (150° 안전 중립)";
        stateTxt.style.color = "var(--accent-red)";
      } else {
        badge.className = "badge badge-run";
        badge.innerHTML = "🟢 전체 정상 가동 중";
        stateTxt.innerText = "RUNNING (실시간 파도)";
        stateTxt.style.color = "var(--accent-green)";
      }
      document.getElementById('txt-score').innerText = data.score.toFixed(1) + " 점";
      document.getElementById('txt-dist').innerText = (data.dist !== undefined ? data.dist.toFixed(2) : "3.00") + " m";
      document.getElementById('txt-damp').innerText = "K = " + (data.damp !== undefined ? data.damp.toFixed(2) : "1.00") + ((data.damp < 0.85) ? " (관심 진정)" : " (날것 데이터)");
      document.getElementById('txt-amp').innerText = "±" + data.amp.toFixed(1) + "° (" + (150 - data.amp).toFixed(1) + "° ~ " + (150 + data.amp).toFixed(1) + "°)";
      document.getElementById('txt-post').innerText = `[${data.time}] "${data.post}"`;
    }

    setInterval(async () => {
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        updateUI(data);
      } catch(e) {}
    }, 1500);
  </script>
</body>
</html>
)rawliteral";

// =========================================================================
// [4. 초음파 센서 계측 및 ESP-NOW 무선 발사 함수]
// =========================================================================
float readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // HC-SR04 에코 수신 (최대 30ms = 약 5.1m 타임아웃)
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0 || duration >= 24000) {
    return 3.0f; // 반사파 미수신(원거리 또는 장애물 없음) 시 즉각 3.0m 반환
  }
  float distMeters = (duration * 0.0343f) / 2.0f / 100.0f;
  if (distMeters < 0.05f || distMeters > 3.5f) {
    return 3.0f;
  }
  return distMeters;
}

float calculateDampingFactor(float dist) {
  // 0.2m(초근접) ~ 2.0m(원거리) 전 구간에 걸쳐 1mm 단위 완전 선형 연속 매핑
  if (dist >= 2.0f) return 1.0f;
  if (dist <= 0.2f) return 0.12f;
  return 0.12f + ((dist - 0.2f) / 1.8f) * 0.88f;
}

void sendEspNowPacket(uint8_t cmdCode = 0) {
  txData.score = isManualAngleMode ? manualAngle : smoothedScore;
  txData.wavePhase = wavePhase;
  txData.dampFactor = currentDampFactor;
  txData.viewerDist = filteredDistanceMeters;
  txData.isPaused = isPaused;
  txData.cmd = isManualAngleMode ? 10 : cmdCode;

  esp_now_send(broadcastAddress, (uint8_t *) &txData, sizeof(txData));
}

void setCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

void handleStatus() {
  setCORS();
  DynamicJsonDocument doc(512);
  doc["role"] = "Master Gateway (C to C)";
  doc["paused"] = isPaused;
  doc["score"] = smoothedScore;
  doc["dist"] = filteredDistanceMeters;
  doc["damp"] = currentDampFactor;
  doc["amp"] = currentAmplitude;
  doc["phase"] = wavePhase;
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
  singleServo.write(BASE_ANGLE);
  sendEspNowPacket(0);
  Serial.println("\n[🛑 전체 긴급 정지] 마스터 & 슬레이브 150도 정지 명령 발사 완료!");
  handleStatus();
}

void handleResume() {
  setCORS();
  isPaused = false;
  sendEspNowPacket(0);
  Serial.println("\n[▶️ 전체 실시간 재가동] 마스터 & 슬레이브 파도 재개 완료!");
  handleStatus();
}

void handleSyncPhase() {
  setCORS();
  wavePhase = 0.0;
  sendEspNowPacket(0);
  Serial.println("\n[🌊 전체 파도 동기화] 마스터 & 슬레이브 위상(wavePhase=0) 칼일치 동기화 완료!");
  handleStatus();
}

void handleSweep() {
  setCORS();
  Serial.println("\n[🎯 전체 자가진단] 마스터 & 슬레이브 진단 스윙 실행");
  sendEspNowPacket(1);
  singleServo.write(165);
  delay(300);
  singleServo.write(135);
  delay(300);
  singleServo.write(BASE_ANGLE);
  handleStatus();
}

void handleReboot() {
  setCORS();
  sendEspNowPacket(2);
  server.send(200, "application/json", "{\"status\":\"rebooting\"}");
  delay(500);
  ESP.restart();
}

// =========================================================================
// [Core 0 백그라운드 태스크: Wi-Fi 통신, 오픈 소셜 수신, 웹서버 & ESP-NOW]
// =========================================================================
void dataFetchTask(void *pvParameters) {
  // 브라운아웃 디텍터 비활성화
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.printf("[Core 0] Wi-Fi '%s' 연결 시작...\n", ssid);
  WiFi.mode(WIFI_STA); // STA 단독 모드로 전력 소모 최소화
  WiFi.begin(ssid, password);

  unsigned long lastFetchTime = 0;

  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!isWifiConnected) {
        isWifiConnected = true;
        localIPStr = WiFi.localIP().toString();

        // 1. ESP-NOW 초기화
        if (esp_now_init() == ESP_OK) {
          Serial.println("[ESP-NOW] 🎉 마스터 무선 통신 엔진 초기화 성공!");

          esp_now_peer_info_t peerInfo = {};
          memcpy(peerInfo.peer_addr, broadcastAddress, 6);
          peerInfo.channel = 0;
          peerInfo.encrypt = false;
          esp_now_add_peer(&peerInfo);
        }

        // 2. mDNS 및 웹서버 시작
        if (MDNS.begin("kinetic-master")) {
          Serial.println("[mDNS] http://kinetic-master.local 등록 완료!");
        }
        MDNS.addService("http", "tcp", 80);

        server.on("/", HTTP_GET, []() {
          server.send_P(200, "text/html", DASHBOARD_HTML);
        });
        server.on("/api/status", HTTP_GET, handleStatus);
        server.on("/api/pause", HTTP_GET, handlePause);
        server.on("/api/resume", HTTP_GET, handleResume);
        server.on("/api/sync_phase", HTTP_GET, handleSyncPhase);
        server.on("/api/sweep", HTTP_GET, handleSweep);
        server.on("/api/reboot", HTTP_GET, handleReboot);
        server.begin();

        Serial.println("\n==================================================");
        Serial.println("  🎉 [마스터 관제 대시보드 서버 준비 완료!]");
        Serial.printf("  >> 브라우저 접속 주소: http://%s\n", localIPStr.c_str());
        Serial.println("  >> 또는: http://kinetic-master.local");
        Serial.println("==================================================");
      }

      server.handleClient();

      // 3.5초마다 글로벌 7대 분쟁 키워드 순환 수집
      if (millis() - lastFetchTime >= 3500) {
        lastFetchTime = millis();

        if (!isPaused) {
          const char* CRISIS_TAGS[] = {"war", "conflict", "ukraine", "iran", "missile", "military", "crisis"};
          const int NUM_TAGS = 7;
          static int tagIdx = 0;
          const char* targetTag = CRISIS_TAGS[tagIdx];
          tagIdx = (tagIdx + 1) % NUM_TAGS;

          WiFiClientSecure client;
          client.setInsecure();

          HTTPClient http;
          String url = "https://mastodon.social/api/v1/timelines/tag/" + String(targetTag) + "?limit=5";

          http.begin(client, url);
          http.setTimeout(3000);
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
                if (cleanText.length() > 55) cleanText = cleanText.substring(0, 55) + "...";
                lastPostSnippet = "#" + String(targetTag) + " | " + cleanText;

                // 만점 기준치를 32%로 대폭 상향하여 일상 점수를 더 차분하게 조정 (일상 28~34점)
                float incomingScore = 20.0f + (posts.size() * 1.15f); // 10건 유입 시 31.5점
                sharedScore = (sharedScore * 0.82f) + (incomingScore * 0.18f);
                sharedScore = constrain(sharedScore, 10.0f, 98.4f);

                Serial.printf("\n[📡 실시간 #%s 수집] 글 수: %d건 | 긴장도: %.1f점 -> 슬레이브 1ms 전송!\n", targetTag, posts.size(), sharedScore);
                sendEspNowPacket(0);
              }
            }
          }
          http.end();
        }
      }
    } else {
      isWifiConnected = false;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// =========================================================================
// [Core 1 메인 태스크: 마스터 서보 구동 및 슬레이브 0.05초 정주기 패킷 전송]
// =========================================================================
void setup() {
  // 브라운아웃 디텍터 비활성화 (모터 전압 강하 재부팅 방지)
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(300);

  Serial.println("\n==================================================");
  Serial.println("  ESP32 [마스터 게이트웨이] C to C 보드");
  Serial.println("  Wi-Fi 수신 + ESP-NOW 슬레이브 초고속 MAC 무선 전송");
  Serial.println("==================================================");

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // 초음파 센서 핀 모드 초기화
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // I2C 및 PCA9685 16채널 서보 드라이버 초기화 (SDA: GPIO 21, SCL: GPIO 22)
  Wire.begin(21, 22);
  if (pwm.begin()) {
    pca9685Found = true;
    pwm.setPWMFreq(50);
    Serial.println("[PCA9685] 🎉 16채널 I2C 서보 드라이버 감지 완료! 3개 서보 50Hz 가동");
    for (uint8_t ch = 0; ch < 3; ch++) {
      pwm.writeMicroseconds(ch, angleToMicros(BASE_ANGLE));
    }
  } else {
    Serial.println("[PCA9685] ⚠️ I2C 서보 드라이버 미응답 (0x40 배선 확인 요망) -> GPIO 18 모드로 동작");
  }

  singleServo.setPeriodHertz(50);
  singleServo.attach(SERVO_PIN, 500, 2500);
  singleServo.write(BASE_ANGLE); // 150도 중립 정지 대기
  delay(100);

  xTaskCreatePinnedToCore(
    dataFetchTask,
    "DataFetchTask",
    12288,
    NULL,
    1,
    NULL,
    0
  );
}

void loop() {
  unsigned long currentMillis = millis();

  // 0. 0.06초(60ms)마다 초음파 관람객 실시간 거리 측정 & 초고속 선형 추종
  if (currentMillis - lastUltrasonicPing >= 60) {
    lastUltrasonicPing = currentMillis;
    float measuredDist = readUltrasonicDistance();

    // 멀어질 때와 다가올 때 모두 튀지 않고 즉각 반응하는 지수 보간
    float alpha = (measuredDist > filteredDistanceMeters) ? 0.45f : 0.35f;
    filteredDistanceMeters = (filteredDistanceMeters * (1.0f - alpha)) + (measuredDist * alpha);
    currentDampFactor = calculateDampingFactor(filteredDistanceMeters);
  }

  // 1. 0.5초마다 점수 스무딩 & 기본 속도 갱신
  if (currentMillis - lastScoreUpdate >= 500) {
    lastScoreUpdate = currentMillis;

    if (!isPaused) {
      float currentTarget = sharedScore;
      smoothedScore = smoothedScore * 0.85f + currentTarget * 0.15f;
      currentSpeed = 1.5f + (smoothedScore / 100.0f) * 3.5f;
    }
  }

  // 2. 1.2초마다 초음파 실측치 시리얼 디버그 출력
  if (currentMillis - lastDebugPrint >= 1200) {
    lastDebugPrint = currentMillis;
    Serial.printf("[📡 초음파] 거리: %.2fm | 감쇄 K: %.2f | 진폭: ±%.1f° | PCA9685: %s\n",
      filteredDistanceMeters, currentDampFactor, currentAmplitude, pca9685Found ? "OK(3개 모터)" : "미감지");
  }

  // 3. 0.05초(50ms)마다 슬레이브들에게 위상 및 감쇄 계수 동기화 패킷 전송
  if (currentMillis - lastEspNowSend >= 50) {
    lastEspNowSend = currentMillis;
    if (isWifiConnected) {
      sendEspNowPacket(0);
    }
  }

  // 4. 초당 50회(20ms) 연속 서보 구동 & 50Hz 프레임 단위 연속 LERP & PCA9685 3채널 위상차 파도
  if (currentMillis - lastMotionUpdate >= 20) {
    lastMotionUpdate = currentMillis;

    if (isPaused) {
      singleServo.write(BASE_ANGLE);
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
      // 50Hz(초당 50회) 프레임마다 진폭을 목표값으로 부드럽게 연속 보간 -> 1mm 단위의 아날로그 곡선!
      float baseAmp = 5.0f + (smoothedScore / 100.0f) * 15.0f;
      float targetAmp = baseAmp * currentDampFactor;
      currentAmplitude += (targetAmp - currentAmplitude) * 0.08f;

      wavePhase += 0.025f * currentSpeed;
      if (wavePhase > 6.28318f * 100.0f) wavePhase = 0.0f;

      // 3개 모터의 공간적 위상차 (Traveling Wave): 채널 0 (0), 채널 1 (-0.25 rad), 채널 2 (-0.50 rad)
      float angle0 = BASE_ANGLE + sin(wavePhase) * currentAmplitude;
      float angle1 = BASE_ANGLE + sin(wavePhase - 0.25f) * currentAmplitude;
      float angle2 = BASE_ANGLE + sin(wavePhase - 0.50f) * currentAmplitude;

      angle0 = constrain(angle0, 130.0f, 170.0f);
      angle1 = constrain(angle1, 130.0f, 170.0f);
      angle2 = constrain(angle2, 130.0f, 170.0f);

      singleServo.write(angle0);
      if (pca9685Found) {
        pwm.writeMicroseconds(0, angleToMicros(angle0));
        pwm.writeMicroseconds(1, angleToMicros(angle1));
        pwm.writeMicroseconds(2, angleToMicros(angle2));
      }
    }
  }

  // 5. 시리얼 모니터 각도 직접 입력 실시간 제어
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();

    if (input.length() > 0) {
      if (input == "auto" || input == "live" || input == "a") {
        isManualAngleMode = false;
        Serial.println(F("\n🟢 [자동 모드 복귀] 실시간 국제정세 긴장도 파도 모드로 복귀했습니다."));
        sendEspNowPacket(0);
      } else if (input == "home" || input == "h") {
        isManualAngleMode = true;
        manualAngle = BASE_ANGLE;
        singleServo.write(BASE_ANGLE);
        if (pca9685Found) {
          for (uint8_t ch = 0; ch < 3; ch++) {
            pwm.writeMicroseconds(ch, angleToMicros(BASE_ANGLE));
          }
        }
        Serial.println(F("\n🎯 [홈 포지션 복귀] 마스터 & 서보모터들을 150도 중립으로 회전했습니다."));
        sendEspNowPacket(10);
      } else {
        float angle = input.toFloat();
        if (angle >= 0.0 && angle <= 180.0) {
          isManualAngleMode = true;
          manualAngle = angle;
          singleServo.write(constrain((int)angle, 0, 180));
          if (pca9685Found) {
            for (uint8_t ch = 0; ch < 3; ch++) {
              pwm.writeMicroseconds(ch, angleToMicros(angle));
            }
          }
          Serial.printf("\n🎯 [절대 각도 수동 제어] 모터들이 %.1f° 위치로 동시 회전했습니다! (복귀는 'auto' 입력)\n", angle);
          sendEspNowPacket(10);
        } else {
          Serial.println(F("\n⚠️ 유효한 각도(0~180) 또는 'auto', 'home'을 입력해주세요!"));
        }
      }
    }
  }
}
