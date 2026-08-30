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
const int SERVO_PIN = 4;        // 서보 신호선: GPIO 4
const float BASE_ANGLE = 150.0; // 기본 중립 위치: 150도

// HC-SR04 초음파 거리 센서 3채널 핀 배선 (플래시/스트랩 핀과 완전히 격리된 중앙 안전 핀)
// [센서 1 중앙]: Trig ➡️ D32 (GPIO 32), Echo ➡️ D33 (GPIO 33)
// [센서 2 좌측]: Trig ➡️ D25 (GPIO 25), Echo ➡️ D26 (GPIO 26)
// [센서 3 우측]: Trig ➡️ D27 (GPIO 27), Echo ➡️ D14 (GPIO 14)
const int TRIG_PIN_1 = 32;        // 센서 1 (중앙): Trig (GPIO 32)
const int ECHO_PIN_1 = 33;        // 센서 1 (중앙): Echo (GPIO 33)

const int TRIG_PIN_2 = 25;        // 센서 2 (좌측): Trig (GPIO 25)
const int ECHO_PIN_2 = 26;        // 센서 2 (좌측): Echo (GPIO 26)

const int TRIG_PIN_3 = 27;        // 센서 3 (우측): Trig (GPIO 27)
const int ECHO_PIN_3 = 14;        // 센서 3 (우측): Echo (GPIO 14)

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
const char* ssid = "Artech";
const char* password = "123456789";

// 슬레이브 보드 고유 MAC 주소 목록 (총 4대 슬레이브 + 브로드캐스트)
uint8_t slaveAddressList[][6] = {
  {0x8C, 0x94, 0xDF, 0x6D, 0x8B, 0xC4}, // Slave #1
  {0x8C, 0x94, 0xDF, 0x6D, 0xF5, 0x34}, // Slave #2
  {0x8C, 0x94, 0xDF, 0xB9, 0xAD, 0x30}, // Slave #3
  {0xB8, 0xD6, 0x1A, 0x65, 0xEA, 0x60}, // Slave #4
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}  // 전체 브로드캐스트
};
const int NUM_SLAVES = sizeof(slaveAddressList) / sizeof(slaveAddressList[0]);

typedef struct __attribute__((packed)) struct_message {
  float score;        // 실시간 국제정세 긴장도 (0 ~ 100)
  float wavePhase;    // 파도 위상 (0.0 ~ 6.28)
  float dampFactor;   // 초음파 거리 감쇄 계수 K (0.15 ~ 1.00)
  float viewerDist;   // 관람객 실측 거리 (m)
  bool isPaused;      // 긴급 정지 여부
  uint8_t cmd;        // 0: 일반, 1: 자가진단 스윙, 2: 재부팅, 10: 수동 각도
} struct_message;

struct_message txData;

// 시나리오별 각도(진폭) 및 각속도 설정 구조체 및 변수
struct ScenarioConfig {
  float amp;   // 스윙 진폭 (±각도)
  float speed; // 각속도 계수 (rad/s 단위에 비례)
};

ScenarioConfig cfgUkraine = {19.8f, 4.9f}; // 시나리오 1 [우크라이나] 기본값
ScenarioConfig cfgIran    = {16.3f, 4.1f}; // 시나리오 2 [이란] 기본값
ScenarioConfig cfgPeace   = {7.3f, 2.0f};  // 평화 상태 기본값

String activeMode = "live"; // "live", "ukraine", "iran", "peace", "custom"
float customAmp = 15.0f;
float customSpeed = 2.5f;

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
      document.getElementById('txt-damp').innerText = "K = " + (data.damp !== undefined ? data.damp.toFixed(2) : "1.00") + ((data.damp > 0.85) ? " (관람객 원거리)" : " (관람객 근접)");
      document.getElementById('txt-amp').innerText = "±" + data.amp.toFixed(1) + "° (" + (150 - data.amp).toFixed(1) + "° ~ " + (150 + data.amp).toFixed(1) + "°)";
      document.getElementById('txt-post').innerText = `[${data.time}] "${data.post}"`;
    }

    setInterval(async () => {
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        updateUI(data);
      } catch(e) {}
    }, 200);
  </script>
</body>
</html>
)rawliteral";

float rawDist1 = 3.0f;
float rawDist2 = 3.0f;
float rawDist3 = 3.0f;

float readSingleSensor(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(4);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // HC-SR04 에코 수신 (최대 10ms = 약 1.7m 타임아웃으로 워치독 기아 방지)
  unsigned long duration = pulseIn(echoPin, HIGH, 10000);
  if (duration == 0 || duration >= 9500) {
    return 3.0f; // 타임아웃 또는 미감지 시 3.0m 반환
  }
  float distMeters = (duration * 0.0343f) / 2.0f / 100.0f;
  if (distMeters < 0.05f || distMeters > 3.2f) {
    return 3.0f;
  }
  return distMeters;
}

float readUltrasonicDistance() {
  // 1. 센서 3개 순차 계측 (1ms 시차로 음파 간섭 완벽 방지)
  rawDist1 = readSingleSensor(TRIG_PIN_1, ECHO_PIN_1);
  delayMicroseconds(1000);
  rawDist2 = readSingleSensor(TRIG_PIN_2, ECHO_PIN_2);
  delayMicroseconds(1000);
  rawDist3 = readSingleSensor(TRIG_PIN_3, ECHO_PIN_3);

  // 2. 3개 센서의 평균값 반환 (이상치 3.0m 타임아웃 값은 제외)
  float sum = 0.0f;
  int validCount = 0;
  if (rawDist1 < 2.90f) { sum += rawDist1; validCount++; }
  if (rawDist2 < 2.90f) { sum += rawDist2; validCount++; }
  if (rawDist3 < 2.90f) { sum += rawDist3; validCount++; }

  if (validCount > 0) {
    return sum / (float)validCount;
  }
  return 3.0f; // 모두 타임아웃 시 3.0m 반환
}

float calculateDampingFactor(float dist) {
  // 2.0m 이상(원거리/아무도 없음): K = 1.00 (날것의 소셜 진폭 100%)
  // 0.2m 이하(초근접/관람객 다가옴): K = 0.12 (관람객 관심으로 12% 진정 감쇄)
  if (dist >= 2.0f) return 1.0f;
  if (dist <= 0.2f) return 0.12f;
  return 0.12f + ((dist - 0.2f) / 1.8f) * 0.88f;
}

void sendEspNowPacket(uint8_t cmdCode = 0) {
  float effectiveAmpToSend = 9.5f;
  if (activeMode == "ukraine") effectiveAmpToSend = cfgUkraine.amp;
  else if (activeMode == "iran") effectiveAmpToSend = cfgIran.amp;
  else if (activeMode == "peace") effectiveAmpToSend = cfgPeace.amp;
  else if (activeMode == "custom") effectiveAmpToSend = customAmp;
  else effectiveAmpToSend = 5.0f + (smoothedScore / 100.0f) * 15.0f;

  float scoreToSend = constrain((effectiveAmpToSend - 5.0f) / 15.0f * 100.0f, 0.0f, 100.0f);

  txData.score = isManualAngleMode ? manualAngle : scoreToSend;
  txData.wavePhase = wavePhase;
  txData.dampFactor = currentDampFactor;
  txData.viewerDist = filteredDistanceMeters;
  txData.isPaused = isPaused;
  txData.cmd = isManualAngleMode ? 10 : cmdCode;

  for (int i = 0; i < NUM_SLAVES; i++) {
    esp_now_send(slaveAddressList[i], (uint8_t *) &txData, sizeof(txData));
  }
}

void setCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

void handleStatus() {
  setCORS();
  DynamicJsonDocument doc(768);
  doc["role"] = "Master Gateway (C to C)";
  doc["paused"] = isPaused;
  doc["mode"] = activeMode;
  doc["score"] = smoothedScore;
  doc["dist"] = filteredDistanceMeters;
  doc["damp"] = currentDampFactor;
  doc["d1"] = rawDist1;
  doc["d2"] = rawDist2;
  doc["d3"] = rawDist3;
  doc["amp"] = currentAmplitude;
  doc["speed"] = currentSpeed;
  doc["phase"] = wavePhase;

  doc["ukraine_amp"] = cfgUkraine.amp;
  doc["ukraine_spd"] = cfgUkraine.speed;
  doc["iran_amp"] = cfgIran.amp;
  doc["iran_spd"] = cfgIran.speed;
  doc["peace_amp"] = cfgPeace.amp;
  doc["peace_spd"] = cfgPeace.speed;
  doc["custom_amp"] = customAmp;
  doc["custom_spd"] = customSpeed;

  doc["is_manual"] = isManualAngleMode;
  doc["manual_angle"] = manualAngle;
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

// 🎛️ 시나리오별 각도(진폭) 및 각속도 튜닝 API
void handleSetConfig() {
  setCORS();
  if (server.hasArg("mode")) {
    String m = server.arg("mode");
    m.toLowerCase();
    
    float amp = server.hasArg("amp") ? server.arg("amp").toFloat() : -1.0f;
    float spd = server.hasArg("speed") ? server.arg("speed").toFloat() : -1.0f;
    bool apply = server.hasArg("apply") ? (server.arg("apply") == "true" || server.arg("apply") == "1") : true;

    if (m == "ukraine") {
      if (amp > 0) cfgUkraine.amp = constrain(amp, 1.0f, 35.0f);
      if (spd > 0) cfgUkraine.speed = constrain(spd, 0.5f, 10.0f);
      if (apply) activeMode = "ukraine";
      Serial.printf("\n[🔴 시나리오 1: 우크라이나] 진폭: ±%.1f°, 각속도: %.1f rad/s 설정 완료!\n", cfgUkraine.amp, cfgUkraine.speed);
    } else if (m == "iran") {
      if (amp > 0) cfgIran.amp = constrain(amp, 1.0f, 35.0f);
      if (spd > 0) cfgIran.speed = constrain(spd, 0.5f, 10.0f);
      if (apply) activeMode = "iran";
      Serial.printf("\n[🟠 시나리오 2: 이란] 진폭: ±%.1f°, 각속도: %.1f rad/s 설정 완료!\n", cfgIran.amp, cfgIran.speed);
    } else if (m == "peace") {
      if (amp > 0) cfgPeace.amp = constrain(amp, 1.0f, 35.0f);
      if (spd > 0) cfgPeace.speed = constrain(spd, 0.5f, 10.0f);
      if (apply) activeMode = "peace";
      Serial.printf("\n[🔵 평화 상태: Peace] 진폭: ±%.1f°, 각속도: %.1f rad/s 설정 완료!\n", cfgPeace.amp, cfgPeace.speed);
    } else if (m == "custom") {
      if (amp > 0) customAmp = constrain(amp, 1.0f, 35.0f);
      if (spd > 0) customSpeed = constrain(spd, 0.5f, 10.0f);
      if (apply) activeMode = "custom";
      Serial.printf("\n[🎚️ 커스텀 슬라이더] 진폭: ±%.1f°, 각속도: %.1f rad/s 설정 완료!\n", customAmp, customSpeed);
    } else if (m == "live") {
      activeMode = "live";
      Serial.println("\n[🟢 실시간 라이브] 오픈 소셜 자동 수집 모드로 복귀!");
    }
  }
  handleStatus();
}

void handleManualAngle() {
  setCORS();
  if (server.hasArg("auto") && (server.arg("auto") == "true" || server.arg("auto") == "1")) {
    isManualAngleMode = false;
    Serial.println("\n[🟢 수동 각도 해제] 자동 파도 모드로 복귀!");
  } else if (server.hasArg("angle")) {
    manualAngle = constrain(server.arg("angle").toFloat(), 0.0f, 180.0f);
    isManualAngleMode = true;
    Serial.printf("\n[🎯 수동 고정 각도] 전체 모터 %.1f도 즉시 고정!\n", manualAngle);
  }
  sendEspNowPacket(10);
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

  unsigned long lastFetchTime = 0;

  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!isWifiConnected) {
        isWifiConnected = true;
        localIPStr = WiFi.localIP().toString();
        Serial.printf("\n[Core 0] 🎉 Wi-Fi 연결 완료! IP: %s\n", localIPStr.c_str());

        // mDNS 시작
        if (MDNS.begin("kinetic-master")) {
          Serial.println("[mDNS] http://kinetic-master.local 등록 완료!");
        }
        MDNS.addService("http", "tcp", 80);

        Serial.println("\n==================================================");
        Serial.println("  🎉 [마스터 관제 대시보드 서버 가동 중!]");
        Serial.printf("  >> 브라우저 접속 주소: http://%s\n", localIPStr.c_str());
        Serial.println("  >> 또는: http://kinetic-master.local");
        Serial.println("==================================================");
      }

      // 15초마다 글로벌 7대 분쟁 키워드 순환 수집 (웹서버 응답 지연 방지)
      if (millis() - lastFetchTime >= 15000) {
        lastFetchTime = millis();

        if (!isPaused && activeMode == "live") {
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

    vTaskDelay(5 / portTICK_PERIOD_MS);
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
  Serial.println("  ESP32 [마스터 게이트웨이]");
  Serial.println("  Wi-Fi 수신 + ESP-NOW 슬레이브 초고속 무선 브로드캐스트");
  Serial.println("==================================================");

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // 초음파 센서 3채널 핀 모드 초기화
  pinMode(TRIG_PIN_1, OUTPUT); pinMode(ECHO_PIN_1, INPUT); digitalWrite(TRIG_PIN_1, LOW);
  pinMode(TRIG_PIN_2, OUTPUT); pinMode(ECHO_PIN_2, INPUT); digitalWrite(TRIG_PIN_2, LOW);
  pinMode(TRIG_PIN_3, OUTPUT); pinMode(ECHO_PIN_3, INPUT); digitalWrite(TRIG_PIN_3, LOW);

  singleServo.setPeriodHertz(50);
  singleServo.attach(SERVO_PIN, 500, 2500);
  singleServo.write(BASE_ANGLE); // 150도 중립 정지 대기
  delay(100);

  // Wi-Fi STA 모드 및 2.4GHz AP 접속 (고정 IP: 192.168.0.14)
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  IPAddress local_IP(192, 168, 0, 14);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.begin(ssid, password);
  Serial.printf("[Wi-Fi] '%s' 2.4GHz 무선 공유기 접속 중", ssid);
  int wifiRetry = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetry < 25) {
    delay(200);
    Serial.print(".");
    wifiRetry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    isWifiConnected = true;
    localIPStr = WiFi.localIP().toString();
    Serial.printf("\n[Wi-Fi] 🎉 공유기 접속 성공! 고정 IP: %s (무선 채널: %d)\n", localIPStr.c_str(), WiFi.channel());
  } else {
    Serial.println("\n[Wi-Fi] ⚠️ 공유기 핸드셰이크 진행 중 (백그라운드 지속 접속)");
  }

  if (esp_now_init() == ESP_OK) {
    Serial.println("[ESP-NOW] 🎉 마스터 무선 통신 엔진 초기화 성공!");
    for (int i = 0; i < NUM_SLAVES; i++) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, slaveAddressList[i], 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        Serial.printf("[ESP-NOW] ✅ 슬레이브 Peer 등록 성공: %02X:%02X:%02X:%02X:%02X:%02X\n",
          slaveAddressList[i][0], slaveAddressList[i][1], slaveAddressList[i][2],
          slaveAddressList[i][3], slaveAddressList[i][4], slaveAddressList[i][5]);
      }
    }
  }

  // 웹서버 라우트 등록 및 시작 (Core 1 메인 루프 연동)
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", DASHBOARD_HTML);
  });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/pause", HTTP_GET, handlePause);
  server.on("/api/resume", HTTP_GET, handleResume);
  server.on("/api/sync_phase", HTTP_GET, handleSyncPhase);
  server.on("/api/sweep", HTTP_GET, handleSweep);
  server.on("/api/set_config", HTTP_GET, handleSetConfig);
  server.on("/api/set_score", HTTP_GET, handleSetConfig);
  server.on("/api/mode", HTTP_GET, handleSetConfig);
  server.on("/api/manual_angle", HTTP_GET, handleManualAngle);
  server.on("/api/reboot", HTTP_GET, handleReboot);
  server.begin();
  Serial.println("[WebServer] 🚀 웹서버 80포트 리스닝 시작 완료!");

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
  server.handleClient();
  unsigned long currentMillis = millis();

  // 0. 0.05초(50ms)마다 3개 초음파 융합 거리 측정 & Fast-Attack 초고속 반응
  if (currentMillis - lastUltrasonicPing >= 50) {
    lastUltrasonicPing = currentMillis;
    float measuredDist = readUltrasonicDistance();

    // 사람이 가까워질 때(거리 감소): 즉시 K 상승 → 종이 활발히 흔들림 (alpha = 0.70)
    // 사람이 멀어질 때(거리 증가): 부드럽게 K 하락 → 종이 서서히 가라앉음 (alpha = 0.20)
    float alpha = (measuredDist < filteredDistanceMeters) ? 0.70f : 0.20f;
    filteredDistanceMeters = (filteredDistanceMeters * (1.0f - alpha)) + (measuredDist * alpha);
    currentDampFactor = calculateDampingFactor(filteredDistanceMeters);
  }

  // 1. 0.5초마다 점수 스무딩 & 기본 속도 갱신
  if (currentMillis - lastScoreUpdate >= 500) {
    lastScoreUpdate = currentMillis;

    if (!isPaused && activeMode == "live") {
      float currentTarget = sharedScore;
      smoothedScore = smoothedScore * 0.85f + currentTarget * 0.15f;
    }
  }

  // 2. 1.2초마다 초음파 실측치 시리얼 디버그 출력
  if (currentMillis - lastDebugPrint >= 1200) {
    lastDebugPrint = currentMillis;
    Serial.printf("[📡 마스터] S1:%.2fm | S2:%.2fm | S3:%.2fm -> 융합거리: %.2fm | K: %.2f | 진폭: ±%.1f°\n",
      rawDist1, rawDist2, rawDist3, filteredDistanceMeters, currentDampFactor, currentAmplitude);
  }

  // 3. 0.05초(50ms)마다 4대 슬레이브 노드로 위상 및 감쇄 계수 동기화 패킷 전송 (무조건 50Hz 송출)
  if (currentMillis - lastEspNowSend >= 50) {
    lastEspNowSend = currentMillis;
    sendEspNowPacket(0);
  }

  // 4. 초당 50회(20ms) 연속 서보 구동 & 50Hz 프레임 단위 연속 LERP
  if (currentMillis - lastMotionUpdate >= 20) {
    lastMotionUpdate = currentMillis;

    if (isPaused) {
      singleServo.write(BASE_ANGLE);
    } else if (isManualAngleMode) {
      int target = constrain((int)manualAngle, 0, 180);
      singleServo.write(target);
    } else {
      float targetBaseAmp = 9.5f;
      float targetBaseSpd = 2.5f;

      if (activeMode == "ukraine") {
        targetBaseAmp = cfgUkraine.amp;
        targetBaseSpd = cfgUkraine.speed;
      } else if (activeMode == "iran") {
        targetBaseAmp = cfgIran.amp;
        targetBaseSpd = cfgIran.speed;
      } else if (activeMode == "peace") {
        targetBaseAmp = cfgPeace.amp;
        targetBaseSpd = cfgPeace.speed;
      } else if (activeMode == "custom") {
        targetBaseAmp = customAmp;
        targetBaseSpd = customSpeed;
      } else { // "live" 실시간 소셜 데이터
        targetBaseAmp = 5.0f + (smoothedScore / 100.0f) * 15.0f;
        targetBaseSpd = 1.5f + (smoothedScore / 100.0f) * 3.5f;
      }

      // 초음파 거리 감쇄 K 적용
      float targetAmp = targetBaseAmp * currentDampFactor;
      float targetSpd = targetBaseSpd * (0.6f + 0.4f * currentDampFactor);

      currentAmplitude += (targetAmp - currentAmplitude) * 0.08f;
      currentSpeed += (targetSpd - currentSpeed) * 0.08f;

      wavePhase += 0.025f * currentSpeed;
      if (wavePhase > 6.2831853f * 100.0f) wavePhase = 0.0f;

      // 마스터 로컬 서보 구동
      float angle0 = BASE_ANGLE + sin(wavePhase) * currentAmplitude;
      angle0 = constrain(angle0, 110.0f, 190.0f);

      singleServo.write(constrain((int)angle0, 0, 180));
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
        Serial.println(F("\n🎯 [홈 포지션 복귀] 마스터 & 서보모터들을 150도 중립으로 회전했습니다."));
        sendEspNowPacket(10);
      } else {
        float angle = input.toFloat();
        if (angle >= 0.0 && angle <= 180.0) {
          isManualAngleMode = true;
          manualAngle = angle;
          singleServo.write(constrain((int)angle, 0, 180));
          Serial.printf("\n🎯 [절대 각도 수동 제어] 모터들이 %.1f° 위치로 동시 회전했습니다! (복귀는 'auto' 입력)\n", angle);
          sendEspNowPacket(10);
        } else {
          Serial.println(F("\n⚠️ 유효한 각도(0~180) 또는 'auto', 'home'을 입력해주세요!"));
        }
      }
    }
  }

  vTaskDelay(1 / portTICK_PERIOD_MS);
}
