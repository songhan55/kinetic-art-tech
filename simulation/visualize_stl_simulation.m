%% STL 하부 구조물 (180도 뒤집힘 보정 & 카메라/좌표계 완전 고정) 3채널 시뮬레이션
% 파일명: visualize_stl_simulation.m
% 명세서: C:\art_tech\project_spec.md

clear; clc; close all;

fprintf('=================================================================\n');
fprintf('  ⚙️ STL 180도 뒤집힘 보정 & 좌표계/카메라 고정 3채널 3D 시뮬레이션\n');
fprintf('=================================================================\n\n');

%% 1. STL 하부 구조물 파일 검증 및 로드
stlPath = 'C:\art_tech\model\pantop3.stl';

if ~exist(stlPath, 'file')
    error('❌ STL 파일을 찾을 수 없습니다: %s', stlPath);
end

stlMesh = stlread(stlPath);
pts = stlMesh.Points;

%% 2. STL 180도 뒤집힘 보정 (Rotation Matrix)
% 기존 -90도에서 반대로 +90도 적용하여 바르게 세움
rotAngleX = 90; % X축 기준 +90도 회전
R_x = [1, 0, 0; 
       0, cosd(rotAngleX), -sind(rotAngleX); 
       0, sind(rotAngleX), cosd(rotAngleX)];

pts_rotated = (R_x * pts')';

% 바닥 Z 좌표가 정확히 0이 되도록 오프셋 재설정
min_z = min(pts_rotated(:, 3));
pts_rotated(:, 3) = pts_rotated(:, 3) - min_z;

% 단위 변환 (mm -> m) 및 스케일 조정 (0.002 = 2mm -> 2cm 단위 적정 크기)
scaleFactor = 0.002;
pts_m = pts_rotated * scaleFactor;

fprintf('✅ STL 180도 뒤집힘 보정 및 지면(Z=0) 고정 완료!\n');

%% 3. 테스트용 3개 모터 위치 배치 (삼각형 형태)
motor_x = [-0.4,  0.4,  0.0];
motor_y = [-0.2, -0.2,  0.5];
num_motors = length(motor_x);
L = 1.0; % 1m 유리섬유

%% 4. MATLAB 3D 캔버스 구축 및 카메라/좌표계 고정 (Lock View)
fig = figure('Name', 'STL 좌표계 완전 고정 3채널 시뮬레이션', ...
    'Position', [100, 100, 1100, 700], 'Color', [0.05, 0.07, 0.12]);

ax = axes('Parent', fig, 'Color', [0.03, 0.04, 0.08]);
hold(ax, 'on'); grid(ax, 'on');

% (중요) 축 범위 및 비율 완전 수동 고정 (Auto-Scale 방지)
xlim(ax, [-1.2, 1.2]);
ylim(ax, [-0.8, 1.2]);
zlim(ax, [0.0, 1.8]);
daspect(ax, [1 1 1]); % X:Y:Z 1:1:1 비율 고정

% 축 모드 수동 고정 (화면 흔들림/재설정 방지)
ax.XLimMode = 'manual';
ax.YLimMode = 'manual';
ax.ZLimMode = 'manual';
ax.CameraPositionMode = 'manual';
ax.CameraTargetMode = 'manual';
ax.CameraViewAngleMode = 'manual';

xlabel(ax, 'X (m)', 'Color', 'w');
ylabel(ax, 'Y (m)', 'Color', 'w');
zlabel(ax, 'Z (m)', 'Color', 'w');
ax.XColor = [0.6 0.7 0.8]; ax.YColor = [0.6 0.7 0.8]; ax.ZColor = [0.6 0.7 0.8];
title(ax, '🌾 STL 구조물 정방향 90도/180도 보정 완료 (고정 3D 시점)', 'Color', [0.2 0.8 1.0], 'FontSize', 13);

% 시점 고정 (View Angle)
view(ax, -37.5, 25);

% (1) 3개 위치에 보정된 STL 하부 구조물 렌더링
for i = 1:num_motors
    shifted_pts = pts_m;
    shifted_pts(:, 1) = shifted_pts(:, 1) + motor_x(i);
    shifted_pts(:, 2) = shifted_pts(:, 2) + motor_y(i);
    
    patch(ax, 'Faces', stlMesh.ConnectivityList, 'Vertices', shifted_pts, ...
        'FaceColor', [0.45 0.55 0.7], 'EdgeColor', 'none', 'FaceAlpha', 0.9);
end

% 3D 조명 설정
camlight(ax, 'headlight');
lighting(ax, 'gouraud');

% (2) 3개 위치에 서보모터 축, 1m 수직 직립 유리섬유, 상단 종 생성
rodHandles = zeros(num_motors, 1);
bellHandles = zeros(num_motors, 1);
struct_height = max(pts_m(:, 3));

for i = 1:num_motors
    % 모터 하우징
    plot3(ax, motor_x(i), motor_y(i), struct_height, 's', ...
        'MarkerSize', 10, 'MarkerFaceColor', [0.2 0.4 0.8], 'MarkerEdgeColor', 'w');
    
    % 1m 수직 직립 유리섬유 (연두색)
    rodHandles(i) = plot3(ax, [motor_x(i), motor_x(i)], [motor_y(i), motor_y(i)], ...
        [struct_height, struct_height + L], '-', 'Color', [0.3 0.9 0.4], 'LineWidth', 3);
    
    % 상단 종 마커 (황금색)
    bellHandles(i) = plot3(ax, motor_x(i), motor_y(i), struct_height + L, ...
        'o', 'MarkerSize', 12, 'MarkerFaceColor', [0.98 0.75 0.15], 'MarkerEdgeColor', [0.85 0.5 0.0]);
end

fprintf('✅ 카메라 시점 및 좌표계 축 완전 고정 완료! 실시간 시뮬레이션 시작...\n');

%% 5. 화면 고정 상태 실시간 파도 시뮬레이션 애니메이션 루프
amp = 20.0;             % 진폭 ±20도
period = 1.2;           % 파도 주기 1.2초
w = 2 * pi / period;
wavelength = 1.0;
k_space = 2 * pi / wavelength;

simTime = 0;
dt = 0.03;

for frame = 1:300
    simTime = simTime + dt;
    
    for i = 1:num_motors
        phase = k_space * (motor_x(i) + motor_y(i));
        
        dev_deg = amp * sin(w * simTime - phase);
        dev_rad = deg2rad(dev_deg);
        
        tipX = motor_x(i) + L * sin(dev_rad);
        tipY = motor_y(i);
        tipZ = struct_height + L * cos(dev_rad);
        
        set(rodHandles(i), 'XData', [motor_x(i), tipX], ...
                           'YData', [motor_y(i), tipY], ...
                           'ZData', [struct_height, tipZ]);
                       
        set(bellHandles(i), 'XData', tipX, 'YData', tipY, 'ZData', tipZ);
    end
    
    drawnow limitrate; % 화면 렌더링 최적화 및 고정 유지
    pause(0.01);
end

fprintf('✅ 시뮬레이션 종료.\n');
