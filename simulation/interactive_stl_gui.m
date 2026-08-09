%% 21채널 3D 키네틱 시뮬레이터 (pantop3.stl 단독 구조물 사용) (MATLAB)
% 파일명: interactive_stl_gui.m
% 명세서: C:\art_tech\project_spec.md
% 구조물: pantop3.stl 단독 사용 (상단 부품 제거됨)

clear; clc; close all;

fprintf('=================================================================\n');
fprintf('  🌾 pantop3.stl 단독 구조물 기반 21채널 3D 시뮬레이터 로드 중...\n');
fprintf('=================================================================\n\n');

%% 1. STL 파일 로드 (pantop3.stl 단독 사용)
stlPath = 'C:\art_tech\model\pantop3.stl';
hasSTL = false;
pts_m = []; struct_height = 0.1; stlMesh = [];

if exist(stlPath, 'file')
    try
        stlMesh = stlread(stlPath);
        pts = stlMesh.Points;
        
        % pantop3.stl 정방향 X축 +90도 회전 보정
        R_x = [1 0 0; 0 cosd(90) -sind(90); 0 sind(90) cosd(90)];
        pts_rotated = (R_x * pts')';
        min_z = min(pts_rotated(:, 3));
        pts_rotated(:, 3) = pts_rotated(:, 3) - min_z;
        pts_m = pts_rotated * 0.002;
        
        struct_height = max(pts_m(:, 3));
        hasSTL = true;
        fprintf('✅ pantop3.stl 하부 구조물 단독 로드 완료! (높이: %.3fm)\n', struct_height);
    catch ME
        fprintf('⚠️ STL 로드 예외: %s\n', ME.message);
    end
end

%% 2. 2.5m 정삼각형 내 21개 모터 삼각 격자 좌표 생성 (6개 행)
S = 2.5;                                % 정삼각형 한 변의 길이 (2.5m)
H = (sqrt(3) / 2) * S;                  % 높이 (약 2.165m)
num_rows = 6;                           % 6개 행 (1+2+3+4+5+6 = 21개)

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

%% 3. 농업용 비닐하우스 FRP 4mm / 1.2m 실물 정밀 물성 계산
L = 1.2;                            % 길이: 1.2m 고정
d = 0.0040;                         % 지름: 4.0mm
r = d / 2;
A = pi * r^2;
I = pi * r^4 / 4;
E = 40e9; rho = 1900; g = 9.81;

m_bell = 0.030;                     % 상단 종 무게 (30g)
m_rod = rho * A * L;                % 4mm 1.2m 유리섬유 무게 (약 28.65g)

k_beam = (3 * E * I) / (L^3);
k_eff = max(0.001, k_beam - (m_bell + 0.5 * m_rod) * g / L);
m_eq = m_bell + (33/140) * m_rod;
zeta = 0.05;
wn = sqrt(k_eff / m_eq);
c_eq = 2 * m_eq * wn * zeta;

num_pts = 30;
s_vec = linspace(0, L, num_pts)';   % 0 ~ 1.2m 세그먼트

%% 4. 표준 MATLAB Figure 창 생성
fig = figure('Name', '🌾 21채널 2.5m 정삼각형 3D 키네틱 시뮬레이터 (pantop3 단독)', ...
    'NumberTitle', 'off', 'Position', [100, 80, 1250, 750], 'Color', [0.05, 0.07, 0.12]);

ax = axes('Parent', fig, 'Position', [0.28, 0.10, 0.68, 0.85], 'Color', [0.03, 0.04, 0.08]);
hold(ax, 'on'); ax.XGrid = 'on'; ax.YGrid = 'on'; ax.ZGrid = 'on';

xlim(ax, [-1.6, 1.6]); ylim(ax, [-0.4, 2.4]); zlim(ax, [0.0, 2.0]);
daspect(ax, [1 1 1]);
ax.XLimMode = 'manual'; ax.YLimMode = 'manual'; ax.ZLimMode = 'manual';

xlabel(ax, 'X (m)', 'Color', 'w'); ylabel(ax, 'Y (m)', 'Color', 'w'); zlabel(ax, 'Z (m)', 'Color', 'w');
ax.XColor = [0.6 0.7 0.8]; ax.YColor = [0.6 0.7 0.8]; ax.ZColor = [0.6 0.7 0.8];
title(ax, '🌾 pantop3.stl 단독 구조물 기반 21채널 3D 공간 파도 시뮬레이션', 'Color', [0.2 0.8 1.0], 'FontSize', 13);
view(ax, -37.5, 25);

% 마우스 드래그 3D 시점 회전 활성화
rotate3d(ax, 'on');

% 2.5m 정삼각형 외곽 가이드 라인 렌더링
triX = [0, -S/2, S/2, 0];
triY = [H, 0, 0, H];
triZ = [0, 0, 0, 0];
plot3(ax, triX, triY, triZ, '--', 'Color', [0.2 0.6 0.9 0.4], 'LineWidth', 1.5);

% pantop3 STL 구조물 21개 렌더링
for i = 1:num_motors
    if hasSTL
        shifted_pts = pts_m;
        shifted_pts(:, 1) = shifted_pts(:, 1) + motor_x(i);
        shifted_pts(:, 2) = shifted_pts(:, 2) + motor_y(i);
        patch(ax, 'Faces', stlMesh.ConnectivityList, 'Vertices', shifted_pts, ...
            'FaceColor', [0.45 0.55 0.7], 'EdgeColor', 'none', 'FaceAlpha', 0.85);
    else
        [bx, by, bz] = cylinder(0.07, 16);
        surf(ax, bx + motor_x(i), by + motor_y(i), bz * 0.1, ...
            'FaceColor', [0.3 0.4 0.5], 'EdgeColor', 'none');
    end
end

camlight(ax, 'headlight'); lighting(ax, 'gouraud');

rodHandles = zeros(num_motors, 1);
bellHandles = zeros(num_motors, 1);

for i = 1:num_motors
    plot3(ax, motor_x(i), motor_y(i), struct_height, 's', ...
        'MarkerSize', 6, 'MarkerFaceColor', [0.2 0.4 0.8], 'MarkerEdgeColor', 'w');
    
    rodHandles(i) = plot3(ax, repmat(motor_x(i), num_pts, 1), repmat(motor_y(i), num_pts, 1), ...
        struct_height + s_vec, '-', 'Color', [0.3 0.9 0.4], 'LineWidth', 1.2);
    
    bellHandles(i) = plot3(ax, motor_x(i), motor_y(i), struct_height + L, ...
        'o', 'MarkerSize', 7, 'MarkerFaceColor', [0.98 0.75 0.15], 'MarkerEdgeColor', [0.85 0.5 0.0]);
end

%% 5. 좌측 컨트롤 UI 제어판
uicontrol('Style', 'text', 'String', '🎬 비디오 재생 & MP4 녹화', ...
    'Position', [20, 680, 230, 25], 'BackgroundColor', [0.09, 0.12, 0.18], ...
    'ForegroundColor', [0.98, 0.75, 0.15], 'FontSize', 11, 'FontWeight', 'bold');

btnPlay = uicontrol('Style', 'pushbutton', 'String', '▶ 재생', ...
    'Position', [20, 635, 70, 38], 'BackgroundColor', [0.1, 0.6, 0.3], ...
    'ForegroundColor', 'w', 'FontSize', 10, 'FontWeight', 'bold');

btnPause = uicontrol('Style', 'pushbutton', 'String', '⏸ 일시정지', ...
    'Position', [95, 635, 75, 38], 'BackgroundColor', [0.6, 0.4, 0.1], ...
    'ForegroundColor', 'w', 'FontSize', 10);

btnReset = uicontrol('Style', 'pushbutton', 'String', '↺ 처음으로', ...
    'Position', [175, 635, 75, 38], 'BackgroundColor', [0.2, 0.3, 0.4], ...
    'ForegroundColor', 'w', 'FontSize', 10);

% 🎥 MP4 동영상 저장 버튼
btnRecord = uicontrol('Style', 'pushbutton', 'String', '🎥 10초 MP4 동영상 저장', ...
    'Position', [20, 595, 230, 34], 'BackgroundColor', [0.7, 0.2, 0.2], ...
    'ForegroundColor', 'w', 'FontSize', 10, 'FontWeight', 'bold');

lblTime = uicontrol('Style', 'text', 'String', '⏱️ 시간: 0.00초 (정지됨)', ...
    'Position', [20, 560, 230, 22], 'BackgroundColor', [0.05, 0.07, 0.12], ...
    'ForegroundColor', 'w', 'FontSize', 10);

sldTimeline = uicontrol('Style', 'slider', 'Min', 0, 'Max', 10, 'Value', 0, ...
    'Position', [20, 535, 230, 18]);

% 3D 시점 및 흔들림 방향 제어
uicontrol('Style', 'text', 'String', '↕️ 흔들림 방향 선택', ...
    'Position', [20, 500, 230, 20], 'BackgroundColor', [0.09, 0.12, 0.18], ...
    'ForegroundColor', [0.3, 0.8, 1.0], 'FontSize', 9, 'FontWeight', 'bold');

popDir = uicontrol('Style', 'popupmenu', ...
    'String', {'↕️ 앞뒤 방향 흔들림 (Y축 - 정면 기준)', '↔️ 좌우 방향 흔들림 (X축)', '↗️ 대각선 방향 흔들림'}, ...
    'Position', [20, 472, 230, 26], 'BackgroundColor', [0.15, 0.2, 0.3], ...
    'ForegroundColor', 'w', 'FontSize', 9);

% 3D 시점 버튼
btnView3D = uicontrol('Style', 'pushbutton', 'String', '3D 입체', 'Position', [20, 442, 52, 25], ...
    'Callback', @(~,~) view(ax, -37.5, 25));
btnViewFront = uicontrol('Style', 'pushbutton', 'String', '정면', 'Position', [77, 442, 52, 25], ...
    'Callback', @(~,~) view(ax, 0, 0));
btnViewSide = uicontrol('Style', 'pushbutton', 'String', '측면', 'Position', [134, 442, 52, 25], ...
    'Callback', @(~,~) view(ax, 90, 0));
btnViewTop = uicontrol('Style', 'pushbutton', 'String', '조감도(위)', 'Position', [191, 442, 58, 25], ...
    'Callback', @(~,~) view(ax, 0, 90));

% 바람 시나리오 모드 선택
uicontrol('Style', 'text', 'String', '📌 21채널 바람 시나리오 선택', ...
    'Position', [20, 405, 230, 22], 'BackgroundColor', [0.09, 0.12, 0.18], ...
    'ForegroundColor', [0.3, 0.8, 1.0], 'FontSize', 10, 'FontWeight', 'bold');

popMode = uicontrol('Style', 'popupmenu', ...
    'String', {'🍃 1) 미풍 모드 (은은한 공간파도 시차, ±4°, 3.5초)', ...
               '🌾 2) 약풍 모드 (21채널 잔물결 공간파도, ±8°, 2.5초)', ...
               '🤝 3) 동시 미풍 모드 (21개 모터 일제히 동시 미풍 휨, ±4°, 3.5초)'}, ...
    'Position', [20, 375, 230, 28], 'BackgroundColor', [0.15, 0.2, 0.3], ...
    'ForegroundColor', 'w', 'FontSize', 9);

uicontrol('Style', 'text', 'String', '📊 21개 모터 공간 삼각 격자 (6행)', ...
    'Position', [20, 335, 230, 22], 'BackgroundColor', [0.05, 0.07, 0.12], ...
    'ForegroundColor', [0.4, 0.9, 0.5], 'FontSize', 9);

%% 6. 21채널 실물 물리 미분방정식 공간 파도 루프
global state;
state.isPlaying = false;
state.currentTime = 0.0;

x_def = zeros(num_motors, 1);
v_def = zeros(num_motors, 1);

btnPlay.Callback = @(~,~) playAction(btnPlay, lblTime);
btnPause.Callback = @(~,~) pauseAction(btnPlay, lblTime);
btnReset.Callback = @(~,~) resetAction(btnPlay, sldTimeline, lblTime);
sldTimeline.Callback = @(src, ~) seekAction(src, lblTime);
btnRecord.Callback = @(~,~) recordMP4(fig, ax, popMode, popDir, motor_x, motor_y, num_motors, L, m_eq, c_eq, k_eff, s_vec, num_pts, struct_height, rodHandles, bellHandles);

dt = 0.03;
wavelength = 1.2;
k_space = 2 * pi / wavelength;

while ishandle(fig)
    if state.isPlaying
        state.currentTime = state.currentTime + dt;
        if state.currentTime > 10.0
            state.currentTime = 0.0;
        end
        set(sldTimeline, 'Value', state.currentTime);
        set(lblTime, 'String', sprintf('⏱️ 시간: %.2f / 10.0초 (▶ 재생 중)', state.currentTime));
    end
    
    t_val = state.currentTime;
    modeIdx = get(popMode, 'Value');
    dirIdx = get(popDir, 'Value');
    
    if modeIdx == 1
        amp = 4.0; w = 2*pi/3.5; isSync = false;
    elseif modeIdx == 2
        amp = 8.0; w = 2*pi/2.5; isSync = false;
    else
        amp = 4.0; w = 2*pi/3.5; isSync = true;
    end
    
    angles = zeros(num_motors, 1);
    
    for i = 1:num_motors
        if isSync
            phase = 0;
        else
            phase = k_space * (motor_x(i) + motor_y(i));
        end
        
        if state.isPlaying
            angles(i) = 150.0 + amp * sin(w * t_val - phase);
        else
            angles(i) = 150.0;
        end
        
        alpha = deg2rad(amp * (-w^2 * sin(w * t_val - phase)));
        F_ext = -m_eq * L * alpha;
        
        a_acc = (F_ext - c_eq * v_def(i) - k_eff * x_def(i)) / m_eq;
        v_def(i) = v_def(i) + a_acc * dt;
        x_def(i) = x_def(i) + v_def(i) * dt;
        
        dev_deg = angles(i) - 150.0;
        dev_rad = deg2rad(dev_deg);
        
        s_norm = s_vec / L;
        bending_shape = 1.5 * (s_norm.^2) - 0.5 * (s_norm.^3);
        
        disp_s = s_vec * sin(dev_rad) + x_def(i) * bending_shape;
        dz_factor = sqrt(max(0, 1 - (disp_s / L).^2));
        
        if dirIdx == 1
            curveX = repmat(motor_x(i), num_pts, 1);
            curveY = motor_y(i) + disp_s;
        elseif dirIdx == 2
            curveX = motor_x(i) + disp_s;
            curveY = repmat(motor_y(i), num_pts, 1);
        else
            curveX = motor_x(i) + disp_s * 0.707;
            curveY = motor_y(i) + disp_s * 0.707;
        end
        
        curveZ = struct_height + s_vec .* dz_factor * cos(dev_rad);
        
        set(rodHandles(i), 'XData', curveX, 'YData', curveY, 'ZData', curveZ);
        set(bellHandles(i), 'XData', curveX(end), 'YData', curveY(end), 'ZData', curveZ(end));
    end
    
    drawnow limitrate;
    pause(dt);
end

function playAction(bPlay, lbl)
    global state; state.isPlaying = true;
    set(bPlay, 'BackgroundColor', [0.1, 0.8, 0.4]);
    set(lbl, 'String', '⏱️ 시간: (▶ 재생 중...)');
end

function pauseAction(bPlay, lbl)
    global state; state.isPlaying = false;
    set(bPlay, 'BackgroundColor', [0.1, 0.6, 0.3]);
    set(lbl, 'String', '⏱️ 시간: (⏸ 일시정지됨)');
end

function resetAction(bPlay, sldTL, lbl)
    global state; state.isPlaying = false; state.currentTime = 0.0;
    state.currentTime = 0.0;
    set(sldTL, 'Value', 0.0); set(bPlay, 'BackgroundColor', [0.1, 0.6, 0.3]);
    set(lbl, 'String', '⏱️ 시간: 0.00초 (정지됨)');
end

function seekAction(sldTL, lbl)
    global state; state.currentTime = get(sldTL, 'Value');
    set(lbl, 'String', sprintf('⏱️ 시간: %.2f초 (탐색됨)', state.currentTime));
end

%% 🎥 MP4 동영상 자동 녹화 함수
function recordMP4(fig, ax, popMode, popDir, mx, my, num_m, L, m_eq, c_eq, k_eff, s_vec, num_pts, h_base, rods, bells)
    global state;
    state.isPlaying = false;
    
    mp4File = 'C:\art_tech\simulation\kinetic_art_simulation.mp4';
    fprintf('🎥 MP4 녹화 시작: %s (10초, 30fps 고화질)\n', mp4File);
    
    msgbox({'🎥 10초 MP4 동영상 녹화를 시작합니다.', '완료 시 안내 창이 뜹니다. 잠시만 기다려주세요!'}, 'MP4 녹화 시작');
    
    v = VideoWriter(mp4File, 'MPEG-4');
    v.FrameRate = 30;
    v.Quality = 95;
    open(v);
    
    fps = 30;
    totalFrames = 300;
    dt_rec = 1 / fps;
    
    modeIdx = get(popMode, 'Value');
    dirIdx = get(popDir, 'Value');
    
    if modeIdx == 1
        amp = 4.0; w = 2*pi/3.5; isSync = false;
    elseif modeIdx == 2
        amp = 8.0; w = 2*pi/2.5; isSync = false;
    else
        amp = 4.0; w = 2*pi/3.5; isSync = true;
    end
    
    k_space = 2 * pi / 1.2;
    x_def = zeros(num_m, 1); v_def = zeros(num_m, 1);
    
    for f = 1:totalFrames
        t_rec = (f - 1) * dt_rec;
        
        for i = 1:num_m
            if isSync
                phase = 0;
            else
                phase = k_space * (mx(i) + my(i));
            end
            
            targetAngle = 150.0 + amp * sin(w * t_rec - phase);
            
            alpha = deg2rad(amp * (-w^2 * sin(w * t_rec - phase)));
            F_ext = -m_eq * L * alpha;
            
            a_acc = (F_ext - c_eq * v_def(i) - k_eff * x_def(i)) / m_eq;
            v_def(i) = v_def(i) + a_acc * dt_rec;
            x_def(i) = x_def(i) + v_def(i) * dt_rec;
            
            dev_deg = targetAngle - 150.0;
            dev_rad = deg2rad(dev_deg);
            
            s_norm = s_vec / L;
            bending_shape = 1.5 * (s_norm.^2) - 0.5 * (s_norm.^3);
            disp_s = s_vec * sin(dev_rad) + x_def(i) * bending_shape;
            dz_factor = sqrt(max(0, 1 - (disp_s / L).^2));
            
            if dirIdx == 1
                curveX = repmat(mx(i), num_pts, 1);
                curveY = my(i) + disp_s;
            elseif dirIdx == 2
                curveX = mx(i) + disp_s;
                curveY = repmat(my(i), num_pts, 1);
            else
                curveX = mx(i) + disp_s * 0.707;
                curveY = my(i) + disp_s * 0.707;
            end
            
            curveZ = h_base + s_vec .* dz_factor * cos(dev_rad);
            
            set(rods(i), 'XData', curveX, 'YData', curveY, 'ZData', curveZ);
            set(bells(i), 'XData', curveX(end), 'YData', curveY(end), 'ZData', curveZ(end));
        end
        
        drawnow;
        frameData = getframe(fig);
        writeVideo(v, frameData);
    end
    
    close(v);
    fprintf('✅ MP4 동영상 저장 완료: %s\n', mp4File);
    msgbox({'✅ 10초 MP4 동영상 저장이 완료되었습니다!', ['저장 위치: ', mp4File]}, 'MP4 저장 성공');
end
