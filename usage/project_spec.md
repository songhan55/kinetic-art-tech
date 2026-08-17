# 🕊️ 현재의 상황을 바탕으로 가까운 미래를 예측하는 장치
## (Device for Predicting the Near Future Based on the Present Situation)
### 키네틱 데이터 아트 전시 명세서 & 시스템 아키텍처

---

## 1. 프로젝트 개요 및 컨셉 (Concept & Philosophy)

본 작품은 **"현재의 실시간 글로벌 빅데이터와 소셜 미디어(Bluesky)의 전쟁 언급량을 실시간 수집하여, 가까운 미래의 전쟁 위험도를 36개의 키네틱 종 파도와 3D 미디어아트로 가시화하고, 관람객의 접근을 통해 '전쟁 억제력(War Deterrence)'을 증명하는 감응형 키네틱 미디어아트"**입니다.

1. **실시간 트렌드 데이터 수신**: Bluesky(블루스카이) 실시간 오픈 소셜 언급량 및 글로벌 7대 분쟁 키워드(war, conflict, ukraine, iran, missile, military, crisis)의 실시간 지수($S(t)$)를 수집합니다.
2. **미래 위험도 예측의 시·청각화**: 36개의 키네틱 막대(1m)와 끝단의 종이 실시간 위기 점수에 비례하여 격렬하게 물결치며 경각심의 소리를 냅니다.
3. **전쟁 억제력(War Deterrence) 인터랙션**: 관람객이 비극의 현실에 관심을 가지고 다가설수록, **마스터 보드에 장착된 초음파 센서 감응**을 통해 모터 파도와 3D 영상이 잔잔한 침묵(평화)으로 전환됩니다.

---

## 2. 공간 레이아웃 및 기구부 물리적 사양

* **전시 공간**: **한 변이 2.5m인 정삼각형(Equilateral Triangle) 바닥 영역**
* **모터 축 배치 방향**: **바닥(지면)과 평행하게 횡방향(Horizontal) 설치**
* **물리적 매체 (동적 캔틸레버)**:
  * 직경 2.5mm, **길이 1.0m 유리섬유 로드(Fiberglass Rod)** (36개)
  * 로드 팁(끝단): **30g 소형 황동 종 (Small Bell)** 장착
* **서보 모터 사양**:
  * **MG966 / MG996R 메탈기어 고토크 서보 모터** (36개)
* **모터 기준 각도 및 구동 범위**:
  * **기준 중립 홈 각도**: **`150.0°`** (수직 정렬 홈 포지션)
  * **최대 스윙 진폭**: **`±20.0°`** (구동 허용 범위: **`130.0° ~ 170.0°`**)

---

## 3. 임베디드 & 무선 네트워크 아키텍처 (ESP-NOW Hybrid)

```text
[ 💻 노트북 관제 대시보드 (http://192.168.0.20) ]
                     │ (Wi-Fi)
                     ▼
[ 🟢 Master Gateway (C to C / COM15 / MAC: b8:d6:1a:65:ea:60) ] ── (Wi-Fi) ──> [ 🌐 글로벌 오픈 소셜 분쟁 데이터 ]
  ├─ 📡 초음파 거리 센서 (TRIG: GPIO 5, ECHO: GPIO 19)
  │    └─ 관람객 실측 거리(0.3m~4.0m) 실시간 계측 & 감쇄율(K) 연산
  │
  │ ⚡ ESP-NOW MAC 무선 직결 (0.001초 초고속 브로드캐스트)
  ▼
[ 🟣 Slave Receiver (A to C / COM16 / MAC: 8c:94:df:6d:8b:c4) ]
  │ (Wi-Fi 라우터 트래픽 0%, 1ms 패킷 수신)
  ▼
[ 🎯 36개 서보 모터 완벽한 1ms 칼군무 파도 & 관람객 감응 150° 진정 ]
```

* **마스터 게이트웨이 (C-to-C 보드 / COM15 / MAC: `b8:d6:1a:65:ea:60`)**:
  * Wi-Fi를 통해 3.5초 주기로 전 세계 실시간 분쟁 데이터 수집
  * **HC-SR04 초음파 센서로 관람객 거리 실시간 측정 (GPIO 5, GPIO 19)**
  * 웹서버(`http://192.168.0.20`) 호스팅을 통해 노트북 대시보드 및 3D WebGL로 실측 거리/감쇄율 중계
  * 슬레이브 보드로 0.001초(1ms) 무선 ESP-NOW 패킷(점수, 위상, 감쇄율 $K$) 브로드캐스트
* **슬레이브 수신기 (A-to-C 보드 / COM16 / MAC: `8c:94:df:6d:8b:c4`)**:
  * 전시장 Wi-Fi 연결 불필요 (간섭 제로)
  * 마스터가 쏜 무선 패킷 수신 즉시 1ms 오차 없는 완벽한 위상 동기화 모터 구동

---

## 4. 4단계 위험도 색상 스펙트럼 (Color Palette Calibration)

* **1단계: 평화 / 근접 진정 (`0 ~ 25 점`)** $\rightarrow$ **에메랄드 틸 그린 (`#10b981`)**, 잔잔한 미세 스윙 ($\pm 5^\circ$)
* **2단계: 일상 트렌드 (`28 ~ 34 점`, 현재 실시간 라이브 구간)** $\rightarrow$ **따뜻한 샴페인 앰버 골드 (`#cba258`)**, 완만한 파도 ($\pm 9.5^\circ$)
* **3단계: 긴장 고조 (`65 ~ 80 점`, 2024 미국-이란 중동 위기 75.0점)** $\rightarrow$ **타오르는 탠저린 오렌지 (`#f97316`)**, 빠른 파도 ($\pm 15.0^\circ$)
* **4단계: 극단적 전쟁 위기 (`85 ~ 100 점`, 2022 러-우 전면 침공 98.4점)** $\rightarrow$ **핏빛 크림슨 루비 레드 (`#ef4444`)**, 최대 진폭 ($\pm 20.0^\circ$) 격렬 타격

---

## 5. 마스터 ESP32 초음파 센서 인터랙션 시스템 (Ultrasonic Human Sensing)

### 1) 하드웨어 핀 배선 (Master ESP32 WROOM-32)
* **센서 모델**: HC-SR04 (또는 3.3V 호환 HC-SR04P)
* **배선 명세**:
  * **VCC** ➡️ **5V** (또는 3.3V)
  * **GND** ➡️ **GND**
  * **TRIG (트리거 신호 출력)** ➡️ **GPIO 5**
  * **ECHO (에코 수신 입력)** ➡️ **GPIO 19**
  *(서보모터 신호선 `GPIO 18`, I2C 확장 `GPIO 21/22`와 충돌 없는 안전한 범용 핀 배치)*

### 2) 실시간 거리 감쇄 수학적 모델 (Deterrence Damping Curve)
초음파 센서로 관람객의 물리적 거리($D$, 단위: m)를 측정하고 노이즈 필터를 적용합니다.

* **원거리 ($D \ge 2.0\text{m}$ / 무관심 상태)**:
  $$K(D) = 1.00 \quad (\text{감쇄율 0\%, 100\% 날것의 실시간 국제정세 데이터 반영})$$
* **근접 접근 ($0.3\text{m} \le D < 2.0\text{m}$ / 관심과 개입 상태)**:
  $$K(D) = 0.15 + \left( \frac{D - 0.3}{1.7} \right) \times 0.85$$
* **초근접 ($D < 0.3\text{m}$ / 최대 개입 상태)**:
  $$K(D) = 0.15 \quad (\text{최대 85\% 진폭 감쇄, 평화 에메랄드 그린, } \pm 5^\circ \text{ 잔잔한 미세 스윙})$$

### 3) 통신 및 동기화 아키텍처
1. **ESP-NOW 1ms 동기화**: 마스터가 연산한 $K$값을 슬레이브들에게 1ms 내 전송하여 36개 모든 모터가 관객의 발걸음에 맞춰 일제히 진정.
2. **3D WebGL 라이브 연동**: 마스터 웹 API(`/api/status`)를 통해 웹 시각화 화면(`exhibition_visual.html`)도 실제 관람객 거리에 즉각 반응하여 에메랄드 그린빛으로 정화.
3. **과거 역사 시나리오 불변 원칙 (Immutable History)**: 2022 러-우 침공(98.4)이나 2024 미-이란 위기(95.2) 재생 시에는 관람객이 다가서도 $K = 1.00$으로 고정 ("이미 발발한 과거의 비극은 개입 불가").

---

## 6. 소프트웨어 구성 요소

| 파일/경로 | 역할 및 기능 |
| :--- | :--- |
| [`kinetic_realtime_system/control_dashboard.html`](file:///C:/art_tech/kinetic_realtime_system/control_dashboard.html) | 노트북 무선 원격 비상 관제 대시보드 (전체 정지, 재가동, 1ms 동기화) |
| [`kinetic_realtime_system/exhibition_visual.html`](file:///C:/art_tech/kinetic_realtime_system/exhibition_visual.html) | 전시장 전면 28,000개 파티클 3D WebGL 미디어 아트 디스플레이 |
| [`kinetic_realtime_system/team_presentation_deck.html`](file:///C:/art_tech/kinetic_realtime_system/team_presentation_deck.html) | 스타벅스 디자인 시스템 기반 팀 프레젠테이션 슬라이드 덱 |
| [`kinetic_realtime_system/esp32_master_gateway/esp32_master_gateway.ino`](file:///C:/art_tech/kinetic_realtime_system/esp32_master_gateway/esp32_master_gateway.ino) | C-to-C 마스터 게이트웨이 펌웨어 (초음파 센서 + Wi-Fi + 웹서버 + ESP-NOW 송신) |
| [`kinetic_realtime_system/esp32_slave_node/esp32_slave_node.ino`](file:///C:/art_tech/kinetic_realtime_system/esp32_slave_node/esp32_slave_node.ino) | A-to-C 슬레이브 수신기 펌웨어 (ESP-NOW 1ms 직결 수신 + 50Hz 서보) |
| [`servo_test/servo_test.ino`](file:///C:/art_tech/servo_test/servo_test.ino) | 아두이노 우노 D9번 핀 서보모터 등속도 정밀 테스트 스케치 |
| [`usage/HC-SR04.PDF`](file:///C:/art_tech/usage/HC-SR04.PDF) | HC-SR04 초음파 거리 센서 정품 데이터시트 |
