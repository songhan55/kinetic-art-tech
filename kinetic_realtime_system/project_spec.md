# 🕊️ 현재의 상황을 바탕으로 가까운 미래를 예측하는 장치
## (Device for Predicting the Near Future Based on the Present Situation)
### 키네틱 데이터 아트 전시 명세서 & 시스템 아키텍처

---

## 1. 프로젝트 개요 및 컨셉 (Concept & Philosophy)

본 작품은 **"현재의 실시간 글로벌 빅데이터와 소셜 미디어(Bluesky)의 전쟁 언급량을 실시간 수집하여, 가까운 미래의 전쟁 위험도를 36개의 키네틱 종 파도와 3D 미디어아트로 가시화하고, 관람객의 접근을 통해 '전쟁 억제력(War Deterrence)'을 증명하는 감응형 키네틱 미디어아트"**입니다.

1. **실시간 트렌드 데이터 수신**: Bluesky(블루스카이) 실시간 오픈 소셜 언급량 및 글로벌 전체 뉴스 중 전쟁/분쟁 내용이 포함된 실시간 뉴스 비율($S(t)$)을 수집합니다.
2. **미래 위험도 예측의 시·청각화**: 36개의 키네틱 막대(1m)와 끝단의 종이 실시간 위기 점수에 비례하여 격렬하게 물결치며 경각심의 소리를 냅니다.
3. **전쟁 억제력(War Deterrence) 인터랙션**: 관람객이 비극의 현실에 관심을 가지고 다가설수록, 초음파 센서 감응을 통해 파도와 영상이 잔잔한 침묵(평화)으로 전환됩니다.

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
[ 🟢 Master Gateway (C to C / COM15 / MAC: b8:d6:1a:65:ea:60) ] ── (Wi-Fi) ──> [ 🌐 Bluesky 실시간 소셜 데이터 ]
                     │
                     │ ⚡ ESP-NOW MAC 무선 직결 (0.001초 초고속 브로드캐스트)
                     ▼
[ 🟣 Slave Receiver (A to C / COM16 / MAC: 8c:94:df:6d:8b:c4) ]
                     │ (Wi-Fi 라우터 트래픽 0%, 1ms 패킷 수신)
                     ▼
[ 🎯 36개 서보 모터 완벽한 1ms 칼군무 파도 & 150° 비상 파킹 ]
```

* **마스터 게이트웨이 (C-to-C 보드 / COM15 / MAC: `b8:d6:1a:65:ea:60`)**:
  * Wi-Fi를 통해 5초 주기로 전 세계 Bluesky 실시간 글 수집
  * 웹서버(`http://192.168.0.20`) 호스팅을 통해 노트북 대시보드 명령 처리
  * 슬레이브 보드로 0.001초(1ms) 무선 ESP-NOW 패킷 브로드캐스트
* **슬레이브 수신기 (A-to-C 보드 / COM16 / MAC: `8c:94:df:6d:8b:c4`)**:
  * 전시장 Wi-Fi 연결 불필요 (간섭 제로)
  * 마스터가 쏜 무선 패킷 수신 즉시 1ms 오차 없는 완벽한 위상 동기화 모터 구동

---

## 4. 4단계 위험도 색상 스펙트럼 (Color Palette Calibration)

* **1단계: 평화 / 근접 진정 (`0 ~ 30 점`)** $\rightarrow$ **에메랄드 틸 그린 (`#10b981`)**, 잔잔한 미세 스윙 ($\pm 5^\circ$)
* **2단계: 일상 트렌드 (`30 ~ 65 점`, 현재 39.5점/42점 구간)** $\rightarrow$ **따뜻한 샴페인 앰버 골드 (`#cba258`)**, 완만한 파도 ($\pm 11^\circ$)
* **3단계: 긴장 고조 (`65 ~ 85 점`)** $\rightarrow$ **타오르는 탠저린 오렌지 (`#f97316`)**, 빠른 파도 ($\pm 15^\circ$)
* **4단계: 극단적 전쟁 위기 (`85 ~ 100 점`)** $\rightarrow$ **핏빛 크림슨 루비 레드 (`#ef4444`)**, 최대 진폭 ($\pm 20^\circ$) 격렬 타격
  * **2022.02 러시아-우크라이나 전면 침공 직전** : **`98.4점`** (GDELT 글로벌 뉴스 빅데이터 피크)
  * **2024.04 미국-이란 중동 확전 위기 직전** : **`95.2점`** (Bluesky / 오픈 소셜 빅데이터 피크)

---

## 5. 소프트웨어 구성 요소

| 파일/경로 | 역할 및 기능 |
| :--- | :--- |
| [`control_dashboard.html`](file:///C:/art_tech/control_dashboard.html) | 노트북 무선 원격 비상 관제 대시보드 (전체 정지, 재가동, 1ms 동기화) |
| [`exhibition_visual.html`](file:///C:/art_tech/exhibition_visual.html) | 전시장 전면 28,000개 파티클 3D WebGL 미디어 아트 디스플레이 |
| [`team_presentation_deck.html`](file:///C:/art_tech/team_presentation_deck.html) | 스타벅스 디자인 시스템 기반 팀 프레젠테이션 슬라이드 덱 |
| [`esp32_master_gateway.ino`](file:///C:/art_tech/esp32_master_gateway/esp32_master_gateway.ino) | C-to-C 마스터 게이트웨이 펌웨어 (Wi-Fi + 웹서버 + ESP-NOW 송신) |
| [`esp32_slave_node.ino`](file:///C:/art_tech/esp32_slave_node/esp32_slave_node.ino) | A-to-C 슬레이브 수신기 펌웨어 (ESP-NOW 1ms 직결 수신 + 50Hz 서보) |
| [`work_log_20260814.md`](file:///C:/art_tech/work_log_20260814.md) | 2026-08-14 일일 개발 일지 |
| [`art_tech_deploy.zip`](file:///C:/art_tech/art_tech_deploy.zip) | 전시 웹 시스템 전체 최신 배포 아카이브 |
