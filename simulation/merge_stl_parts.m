%% pantop3.stl + tilttopv300.stl (상단 부품 Y축 90도 단독 회전 결합)
% 파일명: merge_stl_parts.m
% 기능: 상단 부품(tilttopv300) Y축 90도 단독 회전 후 뿔 결합부에 정밀 오버랩 조립

clear; clc; close all;

fprintf('=================================================================\n');
fprintf('  🛠️ 윗 부품 [Y축 90도 단독] 회전 & 뿔 결합부 오버랩 조립 중...\n');
fprintf('=================================================================\n\n');

%% 1. 두 STL 파일 로드
file1 = 'C:\art_tech\model\pantop3.stl';
file2 = 'C:\art_tech\model\tilttopv300.stl';

if ~exist(file1, 'file') || ~exist(file2, 'file')
    error('❌ STL 파일이 존재하지 않습니다. 경로를 확인해주세요.');
end

fprintf('📂 1) pantop3.stl 로딩 중...\n');
stl1 = stlread(file1);
pts1 = stl1.Points; faces1 = stl1.ConnectivityList;

fprintf('📂 2) tilttopv300.stl 로딩 중...\n');
stl2 = stlread(file2);
pts2 = stl2.Points; faces2 = stl2.ConnectivityList;

%% 2. pantop3.stl 하부 구조물 90도 회전 보정
R_x1 = [1 0 0; 0 cosd(90) -sind(90); 0 sind(90) cosd(90)];
pts1_rot = (R_x1 * pts1')';
min_z1 = min(pts1_rot(:, 3));
pts1_rot(:, 3) = pts1_rot(:, 3) - min_z1; % 바닥 Z=0 고정

max_z1 = max(pts1_rot(:, 3));
center1 = mean(pts1_rot(:, 1:2), 1);

%% 3. tilttopv300.stl 상단 부품 Y축 90도 단독 회전 & 뿔 결합 정렬
rotX2 = 0; 
rotY2 = 90; % Y축 기준 90도 단독 회전
rotZ2 = 0;

R_x2 = [1 0 0; 0 cosd(rotX2) -sind(rotX2); 0 sind(rotX2) cosd(rotX2)];
R_y2 = [cosd(rotY2) 0 sind(rotY2); 0 1 0; -sind(rotY2) 0 cosd(rotY2)];
R_z2 = [cosd(rotZ2) -sind(rotZ2) 0; sind(rotZ2) cosd(rotZ2) 0; 0 0 1];
R2 = R_z2 * R_y2 * R_x2;

pts2_rot = (R2 * pts2')';

% 뿔 오버랩 정렬
center2_curr = mean(pts2_rot, 1);
min_z2_curr = min(pts2_rot(:, 3));

overlapOffsetZ = -10.0; % 10mm 겹침 밀착

offsetX = center1(1) - center2_curr(1);
offsetY = center1(2) - center2_curr(2);
offsetZ = (max_z1 - min_z2_curr) + overlapOffsetZ; 

pts2_aligned = pts2_rot;
pts2_aligned(:, 1) = pts2_aligned(:, 1) + offsetX;
pts2_aligned(:, 2) = pts2_aligned(:, 2) + offsetY;
pts2_aligned(:, 3) = pts2_aligned(:, 3) + offsetZ;

%% 4. 두 STL 삼각 메쉬 하나로 병합 (Concatenation)
combined_vertices = [pts1_rot; pts2_aligned];
combined_faces = [faces1; faces2 + size(pts1_rot, 1)];

combined_mesh = triangulation(combined_faces, combined_vertices);
fprintf('✅ 윗 부품 [Y축 90도 단독] 회전 및 뿔 결합 조립 완료!\n');

%% 5. 통합 STL 파일 저장 (combined_structure.stl)
savePath = 'C:\art_tech\model\combined_structure.stl';
stlwrite(combined_mesh, savePath);
fprintf('💾 [저장 완료] 통합 STL 파일: %s\n\n', savePath);

%% 6. 조립된 3D 모델 확인 창
fig = figure('Name', '🛠️ 윗 부품 Y축 90도 회전 조립 확인', 'Position', [150, 100, 1000, 700], 'Color', [0.05, 0.07, 0.12]);
ax = axes('Parent', fig, 'Color', [0.03, 0.04, 0.08]);
hold(ax, 'on'); grid(ax, 'on'); axis(ax, 'equal');

% pantop3.stl (하부 - 청회색)
patch(ax, 'Faces', faces1, 'Vertices', pts1_rot, ...
    'FaceColor', [0.4 0.5 0.7], 'EdgeColor', 'none', 'FaceAlpha', 0.9);

% tilttopv300.stl (상단 부품 Y축 90도 회전 - 주황색)
patch(ax, 'Faces', faces2, 'Vertices', pts2_aligned, ...
    'FaceColor', [0.95 0.55 0.2], 'EdgeColor', 'none', 'FaceAlpha', 0.9);

camlight(ax, 'headlight'); lighting(ax, 'gouraud');
view(ax, -37.5, 25);
rotate3d(ax, 'on');

xlabel(ax, 'X (mm)', 'Color', 'w'); ylabel(ax, 'Y (mm)', 'Color', 'w'); zlabel(ax, 'Z (mm)', 'Color', 'w');
title(ax, '🛠️ 윗 부품 [Y축 90도 단독] 회전 & 뿔 결합 정밀 맞춤 완료', 'Color', [0.2 0.8 1.0], 'FontSize', 12);

fprintf('📌 [안내] 3D 창을 마우스로 드래그하여 Y축 90도 회전 조립 상태를 확인하세요!\n');
