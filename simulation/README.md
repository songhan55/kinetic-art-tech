# 🕊️ 36채널 2.5m 정삼각형 키네틱 데이터 아트 물리 시뮬레이션

본 폴더(`C:\art_tech\simulation`)에는 **러시아-우크라이나 전쟁 전사자 데이터 기반 키네틱 아트 전시 프로젝트 명세서([`project_spec.md`](file:///C:/art_tech/project_spec.md))**에 정의된 36채널 2.5m 정삼각형 키네틱 물리 메커니즘을 정밀 수치해석하고 시뮬레이션하는 **MATLAB, Simulink, Python, HTML 소스 코드**가 포함되어 있습니다.

---

## 📁 소스 코드 파일 구성

1. **[`run_kinetic_simulation.m`](file:///C:/art_tech/simulation/run_kinetic_simulation.m)** (MATLAB 정밀 물리 시뮬레이션)
   * 2.5m 정삼각형 삼각 격자(8행 36개 모터)의 2D 공간 좌표 $(x_i, y_i)$ 계산.
   * 1m 수직 직립 유리섬유 자중 및 30g 상단 종 하중에 의한 유효 강성($k_{\text{eff}}$) 반영.
   * 전사자 데이터($N$)에 따른 공간 바람 파수 방정식 연산 및 `ODE45` 수치해석.
   * 3D 모터 공간 파도 서피스 맵 및 36채널 시간 응답 그래프(Figure) 생성.

2. **[`build_simulink_model.m`](file:///C:/art_tech/simulation/build_simulink_model.m)** (Simulink 자동 조립 및 생성 스크립트)
   * 1m 유리섬유 2차 전달함수 $G(s) = \frac{\omega_n^2}{s^2 + 2\zeta\omega_n s + \omega_n^2}$ 기반의 Simulink 블록 다이어그램(`kinetic_art_simulink.slx`) 자동 생성.
   * `Sine Wave` 입력 ➔ `Transfer Fcn` ➔ `Scope` / `To Workspace` 자동 라인 연결.

3. **[`interactive_sim.html`](file:///C:/art_tech/simulation/interactive_sim.html)** (초경량 웹 실시간 공간 파도 시뮬레이터)
   * MATLAB 없이 브라우저(Chrome/Edge)에서 36개 모터의 공간 파도(직선 바람, 동심원, 나선 파도) 및 1m 탄성 출렁임을 0.1초 만에 실시간 시각화.

4. **[`run_kinetic_simulation.py`](file:///C:/art_tech/simulation/run_kinetic_simulation.py)** (파이썬 시뮬레이션 스크립트)
   * NumPy/SciPy/Matplotlib 기반의 36채널 공간 파도 수치해석 시뮬레이션.

---

## 🚀 MATLAB 및 Simulink 실행 안내

### 1. MATLAB 수치해석 시뮬레이션 실행 (`run_kinetic_simulation.m`)
MATLAB을 실행하고 작업 디렉토리를 `C:\art_tech\simulation`으로 설정한 후 명령창(Command Window)에 입력합니다:
```matlab
run_kinetic_simulation
```

### 2. Simulink 모델 파일 생성 및 실행 (`build_simulink_model.m`)
MATLAB 명령창에서 아래 명령을 입력하여 `.slx` 모델을 생성하고 시뮬레이션을 수행합니다:
```matlab
% 1. Simulink 모델 자동 생성 및 저장
build_simulink_model

% 2. GUI 없이 빠른 백그라운드 시뮬레이션 실행 (노트북 멈춤 방지)
simOut = sim('kinetic_art_simulink');

% 3. (선택) Simulink 블록 창 열기
open_system('kinetic_art_simulink');
```

---

## ⚡ 노트북 환경에서 MATLAB을 가볍고 빠르게 돌리는 꿀팁

1. **백그라운드 명령 실행 (`sim` 함수 활용)**
   * 무거운 Simulink GUI 에디터 창을 켜지 않고, 명령창에서 `simOut = sim('kinetic_art_simulink')`을 수행하면 10배 이상 빠르고 멈춤 없이 연산 결과를 얻을 수 있습니다.

2. **Software OpenGL 전환**
   * 노트북 그래픽 드라이버 충돌로 멈추는 경우, MATLAB 명령창에 `opengl('save', 'software')`를 입력하세요.
