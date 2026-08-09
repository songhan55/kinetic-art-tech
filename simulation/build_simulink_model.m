%% 36채널 키네틱 데이터 아트 Simulink 모델 자동 생성 스크립트 (MATLAB)
% 파일명: build_simulink_model.m
% 명세서: C:\art_tech\project_spec.md
% 실행 방법: MATLAB Command Window에서 build_simulink_model 입력 후 실행

clear; clc;

%% 1. 물리 파라미터 자동 로드
% 1m 직립 유리섬유 + 30g 종 유효강성 및 고유진동수 계산
L = 1.0;
d = 0.0025; r = d / 2; A = pi * r^2; I = pi * r^4 / 4;
E = 40e9; rho = 2000; g = 9.81;
m_bell = 0.030; m_rod = rho * A * L;

k_beam = (3 * E * I) / (L^3);
k_eff = max(0.001, k_beam - (m_bell + 0.5 * m_rod) * g / L);
m_eq = m_bell + (33/140) * m_rod;
zeta = 0.05;
wn = sqrt(k_eff / m_eq);
fn = wn / (2 * pi);

modelName = 'kinetic_art_simulink';

% 이미 열려있는 동일 이름 시스템이 있으면 저장하지 않고 닫기
if bdIsLoaded(modelName)
    close_system(modelName, 0);
end

% 새 Simulink 시스템 생성
new_system(modelName);

%% 2. Simulink 블록 생성 및 배치
% (1) Sine Wave (모터 구동 입력: 주기 2.8초, 진폭 ±20도)
theta_amp = 20.0;
T_wave = 2.8;
w_wave = 2 * pi / T_wave;

add_block('simulink/Sources/Sine Wave', [modelName '/Motor_Sine_Input']);
set_param([modelName '/Motor_Sine_Input'], ...
    'Amplitude', num2str(theta_amp), ...
    'Frequency', num2str(w_wave), ...
    'Position', [50, 100, 130, 140]);

% (2) Transfer Fcn (1m 직립 유리섬유 2차 전달함수: G(s) = w_n^2 / (s^2 + 2*zeta*w_n*s + w_n^2))
numStr = sprintf('[%.4f]', wn^2);
denStr = sprintf('[1, %.4f, %.4f]', 2*zeta*wn, wn^2);

add_block('simulink/Continuous/Transfer Fcn', [modelName '/Fiberglass_Dynamics']);
set_param([modelName '/Fiberglass_Dynamics'], ...
    'Numerator', numStr, ...
    'Denominator', denStr, ...
    'Position', [180, 95, 310, 145]);

% (3) Scope (결과 시각화 스코프)
add_block('simulink/Sinks/Scope', [modelName '/Motion_Scope']);
set_param([modelName '/Motion_Scope'], 'Position', [370, 100, 410, 140]);

% (4) To Workspace (시뮬레이션 데이터 워크스페이스 저장)
add_block('simulink/Sinks/To Workspace', [modelName '/To_Workspace']);
set_param([modelName '/To_Workspace'], ...
    'VariableName', 'simout_bell', ...
    'SaveFormat', 'Timeseries', ...
    'Position', [370, 160, 430, 190]);

%% 3. 블록 간 라인(와이어) 연결
add_line(modelName, 'Motor_Sine_Input/1', 'Fiberglass_Dynamics/1');
add_line(modelName, 'Fiberglass_Dynamics/1', 'Motion_Scope/1');
add_line(modelName, 'Fiberglass_Dynamics/1', 'To_Workspace/1');

% 자동 레이아웃 정돈
Simulink.BlockDiagram.arrangeSystem(modelName);

% 모델 파일 저장 (.slx)
modelFilePath = [fileparts(mfilename('fullpath')) filesep modelName '.slx'];
save_system(modelName, modelFilePath);

fprintf('=================================================================\n');
fprintf('  ✅ [성공] Simulink 모델 (%s.slx) 생성이 완료되었습니다!\n', modelName);
fprintf('=================================================================\n');
fprintf('· 저장 경로: %s\n', modelFilePath);
fprintf('· 시스템 고유 진동수(fn): %.2f Hz\n', fn);
fprintf('· 전달함수: G(s) = %.2f / (s^2 + %.2f s + %.2f)\n\n', wn^2, 2*zeta*wn, wn^2);
fprintf('📌 [실행 안내]\n');
fprintf(' 1) MATLAB 명령창에서 "open_system(''%s'')" 입력 시 모델 GUI가 열립니다.\n', modelName);
fprintf(' 2) GUI 없이 빠른 백그라운드 계산을 하려면 "simOut = sim(''%s'')"을 입력하세요.\n', modelName);
fprintf('=================================================================\n');
