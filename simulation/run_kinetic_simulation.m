%% 러-우 전쟁 전사자 데이터 기반 36채널 키네틱 데이터 아트 MATLAB 정밀 물리 시뮬레이션
% 파일명: run_kinetic_simulation.m
% 명세서: C:\art_tech\project_spec.md
% 실물 사양 반영: 농업용/비닐하우스용 FRP 4mm 지름, 1.2m 길이 유리섬유 봉

clear; clc; close all;

fprintf('=================================================================\n');
fprintf('  🕊️ 36채널 (4mm/1.2m 비닐하우스 FRP) MATLAB 정밀 물리 시뮬레이션\n');
fprintf('=================================================================\n\n');

%% 1. 2.5m 정삼각형 및 36개 모터 삼각 격자 좌표 생성 (Triangular Grid)
S = 2.5;                                % 정삼각형 한 변의 길이 (2.5m)
H = (sqrt(3) / 2) * S;                  % 높이 (약 2.165m)
L = 1.2;                                % 비닐하우스 FRP 유리섬유 길이 (1.2m)
BASE_ANGLE = 150.0;                     % 기본 대기 각도 (150°)

num_rows = 8;
motor_x = []; motor_y = [];

for r = 1:num_rows
    count_in_row = r;
    row_y = H * (1 - (r - 1) / (num_rows - 1));
    row_w = S * ((r - 1) / (num_rows - 1));
    
    for i = 1:count_in_row
        if count_in_row == 1
            rx = 0;
        else
            rx = -row_w / 2 + (row_w / (count_in_row - 1)) * (i - 1);
        end
        motor_x = [motor_x; rx];
        motor_y = [motor_y; row_y];
    end
end
num_motors = length(motor_x);

%% 2. 농업용 비닐하우스 FRP 4mm / 1.2m 실물 정밀 물성 계산
g = 9.81;
d = 0.0040;                             % 지름 4mm
r = d / 2; A = pi * r^2; I = pi * r^4 / 4;
E = 40e9; rho = 1900;
m_bell = 0.030;                         % 종 무게 (30g)
m_rod = rho * A * L;                    % 4mm 1.2m 유리섬유 자체 무게 (약 28.65g)

k_beam = (3 * E * I) / (L^3);
k_eff = max(0.001, k_beam - (m_bell + 0.5 * m_rod) * g / L); % 직립 유효 강성 (0.5104 N/m)
m_eq = m_bell + (33/140) * m_rod;                             % 등가 질량 (36.75g)
zeta = 0.05;
wn = sqrt(k_eff / m_eq);
c_eq = 2 * m_eq * wn * zeta;
fn = wn / (2 * pi);                                           % 고유진동수 (0.59 Hz, 고유주기 1.68초)

fprintf('· 유리섬유 규격: 지름 4mm, 길이 1.2m (농업용/비닐하우스용 FRP)\n');
fprintf('· 등가 상단 질량 (m_eq): %.2f g, 직립 유효 강성 (k_eff): %.4f N/m\n', m_eq*1000, k_eff);
fprintf('· 시스템 고유 진동수 (fn): %.2f Hz (고유 주기: %.2f 초)\n\n', fn, 1/fn);

%% 3. 전사자 데이터(N) 시나리오 수치해석 (ODE45)
tspan = 0:0.01:6; % 6초 시뮬레이션

% 시나리오 1: 약한 바람 속 잔잔한 갈대 연출 (N = 120명, 미세 흔들림 ±5°)
[t_low, theta_low, bell_low] = run_wave_ode(120, tspan, motor_x, motor_y, BASE_ANGLE, L, m_eq, c_eq, k_eff);

% 시나리오 2: 강한 파도 및 채찍 연출 (N = 950명, 강한 흔들림 ±20°)
[t_high, theta_high, bell_high] = run_wave_ode(950, tspan, motor_x, motor_y, BASE_ANGLE, L, m_eq, c_eq, k_eff);

%% 4. 정밀 시각화 그래프 (MATLAB Figures)
figure('Name', '4mm 1.2m 비닐하우스 FRP 적용 36채널 키네틱 공간 파도', 'Position', [100, 100, 1000, 600]);

subplot(1, 2, 1);
scatter3(motor_x, motor_y, bell_low(:, end), 60, bell_low(:, end), 'filled');
cb1 = colorbar; cb1.Label.String = '종 변위 (cm)';
grid on; axis equal;
title('🌾 약한 갈대 연출 (N=120) - 36개 종 변위 (4mm/1.2m 적용)');
xlabel('X (m)'); ylabel('Y (m)'); zlabel('종 변위 (cm)');
view(-37.5, 30);

subplot(1, 2, 2);
scatter3(motor_x, motor_y, bell_high(:, end), 60, bell_high(:, end), 'filled');
cb2 = colorbar; cb2.Label.String = '종 변위 (cm)';
grid on; axis equal;
title('⚡ 강한 채찍 연출 (N=950) - 36개 종 변위 (4mm/1.2m 적용)');
xlabel('X (m)'); ylabel('Y (m)'); zlabel('종 변위 (cm)');
view(-37.5, 30);

figure('Name', '4mm 1.2m FRP 적용 36채널 시간 응답 곡선', 'Position', [150, 150, 1000, 700]);

subplot(2, 2, 1);
plot(t_low, theta_low, 'LineWidth', 1.0);
grid on; xlabel('시간 (초)'); ylabel('모터 회전각 (°)');
title('N=120: 36개 모터 미세 각도 (150° ± 5°)');

subplot(2, 2, 2);
plot(t_high, theta_high, 'LineWidth', 1.0);
grid on; xlabel('시간 (초)'); ylabel('모터 회전각 (°)');
title('N=950: 36개 모터 최대 각도 (150° ± 20°)');

subplot(2, 2, 3);
plot(t_low, bell_low, 'LineWidth', 1.2);
grid on; xlabel('시간 (초)'); ylabel('종 상단 변위 (cm)');
title('N=120: 4mm/1.2m 유리섬유 잔잔한 미풍 휨 변위');

subplot(2, 2, 4);
plot(t_high, bell_high, 'LineWidth', 1.2);
grid on; xlabel('시간 (초)'); ylabel('종 상단 변위 (cm)');
title('N=950: 4mm/1.2m 유리섬유 강한 채찍 휨 변위');

fprintf('✅ 4mm/1.2m 실물 물성 전면 적용 정밀 물리 시뮬레이션 완료!\n');

%% 5. 공간 파도 ODE 계산 로컬 함수
function [t_vec, theta_mat, bell_mat] = run_wave_ode(N, tspan, mx, my, base_angle, L, m, c, k)
    num_m = length(mx);
    norm_n = max(0, min(1, (N - 50) / (1200 - 50)));
    amp = 5.0 + norm_n * 15.0;            % 진폭 5° ~ 20°
    period = 3.2 - norm_n * 2.6;          % 주기 3.2s ~ 0.6s
    w = 2 * pi / period;
    
    wavelength = 1.2;
    k_space = 2 * pi / wavelength;
    dir_rad = deg2rad(45);
    kx = k_space * cos(dir_rad);
    ky = k_space * sin(dir_rad);
    
    t_vec = tspan';
    theta_mat = zeros(length(t_vec), num_m);
    bell_mat = zeros(length(t_vec), num_m);
    
    for i = 1:num_m
        phase = kx * mx(i) + ky * my(i);
        
        theta_i = base_angle + amp * sin(w * t_vec - phase);
        theta_mat(:, i) = theta_i;
        
        [~, Y] = ode45(@(t, y) rod_ode_fun(t, y, amp, w, phase, L, m, c, k), t_vec, [0; 0]);
        
        base_pos_cm = L * sin(deg2rad(theta_i - base_angle)) * 100;
        bell_mat(:, i) = base_pos_cm + Y(:, 1) * 100;
    end
end

function dydt = rod_ode_fun(t, y, amp, w, phase, L, m, c, k)
    alpha = deg2rad(amp * (-w^2 * sin(w * t - phase)));
    F_ext = -m * L * alpha;
    
    x = y(1); v = y(2);
    dxdt = v;
    dvdt = (F_ext - c*v - k*x) / m;
    dydt = [dxdt; dvdt];
end
