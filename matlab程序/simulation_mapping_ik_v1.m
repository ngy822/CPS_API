% 4-PR 并联调姿机构：基于实测靶标点云的逆解计算与 3D 可视化
clear; clc; close all;

% ================= 1. 机构与工件理论参数 =================
Rw = 3506 / 2;  % 工件大圆柱半径 (mm)
Lw = 9580;      % 移动段工件长度 (mm)
L_fixed = 4000; % 固定段工件长度 (mm)

Xc = 2710 / 2;  % 支链横向跨距的一半
Yc = 4500 / 2;  % 支链纵向跨距的一半
Hc = 1500;      % 顶升柱初始高度
R_col = 200;    % 顶升柱半径
rr = 850 / 2;   % 滚筒半径
Lr = 350;       % 滚筒长度

% 理论状态下，装备处于零位的世界坐标系 {W_ideal}
T_W_ideal = eye(4);
T_W_ideal(2, 4) = Lw/2; 

% ================= 2. 读取并拟合激光跟踪仪靶标数据 =================
if ~isfile('laser_tracker_data.mat')
    error('未找到 laser_tracker_data.mat，请先运行靶标生成器程序！');
end
load('laser_tracker_data.mat', 'Targets');

% --- 2.1 拟合世界系 {W_fitted} (P1, P2, P3) ---
p1 = Targets(1,:)'; p2 = Targets(2,:)'; p3 = Targets(3,:)';
O_W_f = p2;
Y_W_f = (p1 - p2) / norm(p1 - p2);
Z_W_f = cross(p3 - p2, p1 - p2); Z_W_f = Z_W_f / norm(Z_W_f);
X_W_f = cross(Y_W_f, Z_W_f); X_W_f = X_W_f / norm(X_W_f);
T_W_fitted = [X_W_f, Y_W_f, Z_W_f, O_W_f; 0 0 0 1];

% --- 2.2 拟合固定段端面坐标系 {F_fitted} (P7 ~ P12) ---
% C1: P7, P8, P9 的空间外心; C2: P10, P11, P12 的空间外心
C1_F = fitCircle3D(Targets(7,:)', Targets(8,:)', Targets(9,:)');
C2_F = fitCircle3D(Targets(10,:)', Targets(11,:)', Targets(12,:)');
Y_F_f = (C2_F - C1_F) / norm(C2_F - C1_F); % 轴心向远端延伸方向
O_F_f = C1_F - Y_F_f * 500;                % 沿轴线回退至端面原点
p11 = Targets(11,:)'; 
X_F_temp = (p11 - C2_F) / norm(p11 - C2_F);
X_F_f = X_F_temp - dot(X_F_temp, Y_F_f) * Y_F_f; X_F_f = X_F_f / norm(X_F_f);
Z_F_f = cross(X_F_f, Y_F_f);
T_F_fitted = [X_F_f, Y_F_f, Z_F_f, O_F_f; 0 0 0 1];

% --- 2.3 拟合移动段端面坐标系 {M0_fitted} (P13 ~ P18) ---
C1_M = fitCircle3D(Targets(13,:)', Targets(14,:)', Targets(15,:)');
C2_M = fitCircle3D(Targets(16,:)', Targets(17,:)', Targets(18,:)');
Y_M_f = (C1_M - C2_M) / norm(C1_M - C2_M); % 轴心向远端延伸方向
O_M_f = C1_M + Y_M_f * 500;                % 沿轴线前进至端面原点
p17 = Targets(17,:)';
X_M_temp = (p17 - C2_M) / norm(p17 - C2_M);
X_M_f = X_M_temp - dot(X_M_temp, Y_M_f) * Y_M_f; X_M_f = X_M_f / norm(X_M_f);
Z_M_f = cross(X_M_f, Y_M_f);
T_M0_fitted = [X_M_f, Y_M_f, Z_M_f, O_M_f; 0 0 0 1];

% ================= 3. 空间映射转换 (LT 测量场 -> 设备理论基准场) =================
% 这一步极其关键：将激光跟踪仪任意放置测出的结果，统一转换回求逆解所需的标准正交空间
T_LT2Machine = T_W_ideal * inv(T_W_fitted); 

T_M0 = T_LT2Machine * T_M0_fitted;
T_F_curr = T_LT2Machine * T_F_fitted;

% 同样地，把 18 个测量的靶标点也映射到机器基准场，以备后续可视化
Targets_Machine = zeros(18, 3);
for i = 1:18
    pt_mach = T_LT2Machine * [Targets(i, :)'; 1];
    Targets_Machine(i, :) = pt_mach(1:3)';
end

% ================= 4. 对接匹配策略生成 =================
align_mode = 3; % 选择匹配模式 -> 1:完全重合, 2:仅姿态平行, 3:人工手动设定

if align_mode == 1
    T_Mnew = T_F_curr;
    mode_str = '[模式 1] 完全重合 (目标锁定固定段端面)';
elseif align_mode == 2
    T_Mnew = T_F_curr;
    T_Mnew(1:3, 4) = T_M0(1:3, 4); 
    mode_str = '[模式 2] 仅姿态平行 (保留原地位置)';
elseif align_mode == 3
    % 人工输入 6-DOF 调整指令 (相对于移动段当前位姿)
    man_dx = 150; man_dy = 200; man_dz = 100;
    man_rx = 0.0; man_ry = 0.0; man_rz = 0.0;
    
    T_manual = trans(man_dx, man_dy, man_dz) * ...
               rotX(deg2rad(man_rx)) * rotY(deg2rad(man_ry)) * rotZ(deg2rad(man_rz));
    T_Mnew = T_M0 * T_manual; 
    mode_str = '[模式 3] 人工手动设定调整量';
else
    error('未知的匹配模式！');
end

% 提取逆解所需特征：目标圆心 P_a 和 目标轴线方向向量 v_a
P_a = T_Mnew(1:3, 4); 
v_a = T_Mnew(1:3, 2); 

% ================= 5. 逆向运动学求根 (IK) =================
X_cols = [Xc, Xc, -Xc, -Xc];
Y_cols = [Yc, -Yc, Yc, -Yc];
H_cols = zeros(1, 4);

% 计算基座 Y 向平移量
dy_cmd = T_Mnew(2, 4) - T_M0(2, 4);
Y_cols_new = Y_cols + dy_cmd; 

% 计算固定段补偿 Roll 角度
Delta_T_M = T_M0 \ T_Mnew; 
target_roll_M = rad2deg(asin(Delta_T_M(1,3))); 
roll_cmd_fixed = -target_roll_M; 

% 几何截面求根计算顶升高度
for i = 1:4
    t_i = (Y_cols_new(i) - P_a(2)) / v_a(2);
    X_axis_i = P_a(1) + t_i * v_a(1);
    Z_axis_i = P_a(3) + t_i * v_a(3);
    
    inner_val = (Rw + rr)^2 - (X_cols(i) - X_axis_i)^2;
    if inner_val < 0, error(['幅度超限！支链 ', num2str(i), ' 脱轨！']); end
    
    H_cols(i) = Z_axis_i - sqrt(inner_val);
end

% ================= 6. 控制台数据打印输出 =================
disp('================ 激光跟踪仪测点拟合输出 ================');
printPose('坐标系 2 (实测移动段当前端面 M0)', T_W_ideal \ T_M0);
printPose('坐标系 1 (实测固定段目标端面 F)', T_W_ideal \ T_F_curr);
printPose('>> 最终选定的移动段目标位姿 (Mnew)', T_W_ideal \ T_Mnew);

fprintf('\n================ 匹配与分解运动指令 =================\n');
fprintf('>> 激活对接策略: %s\n\n', mode_str);

fprintf('【固定段工装 (1-DOF)】\n');
fprintf(' 补偿滚动 (Roll) : %+7.2f°\n\n', roll_cmd_fixed);

fprintf('【移动段并联机构 (5-DOF)】\n');
fprintf(' 地轨基座平移 Y  : %+7.2f mm\n', dy_cmd);
for i = 1:4
    fprintf('  - 支链 %d 目标位移: %7.2f mm (变动 %+7.2f mm)\n', i, H_cols(i), H_cols(i) - Hc);
end
disp('=====================================================');

% ================= 7. 3D 可视化 =================
fig = figure('Name', '4-PR 实测点云数据对接与调姿', 'Color', 'w', 'Position', [100, 50, 1400, 950]);
hold on; grid on; axis equal; view(-135, 30);
xlabel('X'); ylabel('Y'); zlabel('Z'); light('Position',[1 1 1],'Style','infinite'); lighting gouraud;
title('实测靶标驱动下的 6-DOF 对接数字孪生系统 (附带靶标点)', 'FontSize', 14);

col_base = [0.7 0.7 0.7]; col_col = [0.2 0.5 0.7]; col_roller = [0.3 0.3 0.3];
col_wire = [0.4 0.4 0.4]; col_wp = [0.9 0.8 0.7]; col_fixed = [0.6 0.8 0.9];

% 1. 基座地轨
drawBox([0, Yc, 0], 5000, 1050, 850, 'none', 1.0, col_wire, '--'); 
drawBox([0, -Yc, 0], 5000, 1050, 850, 'none', 1.0, col_wire, '--');
drawBox([0, Yc + dy_cmd, 0], 5000, 1050, 850, col_base, 0.8, 'k', '-');  
drawBox([0, -Yc + dy_cmd, 0], 5000, 1050, 850, col_base, 0.8, 'k', '-');

% 2. 支链
for i = 1:4
    drawCylinderWireframe([X_cols(i), Y_cols(i), Hc/2], [0 0 1], R_col, Hc, col_wire, '--', 1.5);
    drawCylinderWireframe([X_cols(i), Y_cols(i), Hc], [0 1 0], rr, Lr, col_wire, '--', 1.5);
    drawCylinder([X_cols(i), Y_cols_new(i), H_cols(i)/2], [0 0 1], R_col, H_cols(i), col_col, 0.8);
    drawCylinder([X_cols(i), Y_cols_new(i), H_cols(i)], [0 1 0], rr, Lr, col_roller, 0.9);
end

% 3. 移动段 (初始线框 + 终点实体)
drawCylinderWireframe([0, 0, T_M0(3,4)], [0 1 0], Rw, Lw, col_wire, '--', 1.5);
wp_center = P_a - v_a * (Lw / 2);
drawCylinder(wp_center, v_a, Rw, Lw, col_wp, 0.6); 

% 4. 固定段
P_f = T_F_curr(1:3, 4); v_f = T_F_curr(1:3, 2);
wp_center_fixed = P_f + v_f * (L_fixed / 2);
drawCylinder(wp_center_fixed, v_f, Rw, L_fixed, col_fixed, 0.75); 

% 5. ★ 绘制转换到机器坐标系后的 18 个靶标测点
scatter3(Targets_Machine(1:6, 1), Targets_Machine(1:6, 2), Targets_Machine(1:6, 3), 60, 'r', 'filled', 'MarkerEdgeColor', 'k');
scatter3(Targets_Machine(7:12, 1), Targets_Machine(7:12, 2), Targets_Machine(7:12, 3), 60, 'b', 'filled', 'MarkerEdgeColor', 'k');
scatter3(Targets_Machine(13:18, 1), Targets_Machine(13:18, 2), Targets_Machine(13:18, 3), 60, 'g', 'filled', 'MarkerEdgeColor', 'k');
for i = 1:18
    text(Targets_Machine(i,1)+100, Targets_Machine(i,2), Targets_Machine(i,3)+100, sprintf('P%d', i), 'FontSize', 9, 'Color', [0.2 0.2 0.2], 'FontWeight', 'bold');
end

% 6. 坐标系
T_B = eye(4); T_B(2, 4) = dy_cmd; 
drawFrame(T_W_ideal, 1500, 'W', 1.0);     
drawFrame(T_B, 1500, 'B', 1.0);          
drawFrame(T_F_curr, 2200, 'F', 0.85);   
drawFrame(T_M0, 1100, 'M0', 0.3);        
drawFrame(T_Mnew, 1400, 'M_{new}', 1.0); 

axis([-4000 4000 -7000 11000 0 6000]); camtarget([0 2000 2000]);

% ================= 底层数学与绘图组件 =================
% 【核心组件】：空间三点求外接圆圆心解析算法
function center = fitCircle3D(p1, p2, p3)
    v1 = p2 - p1;
    v2 = p3 - p1;
    n = cross(v1, v2);
    cross_part = cross( (norm(v1)^2 * v2 - norm(v2)^2 * v1), n );
    center = p1 + cross_part / (2 * norm(n)^2);
end

function printPose(title_str, T)
    fprintf('【%s】\n', title_str);
    ry = asin(T(1,3)); rx = atan2(-T(2,3), T(3,3)); rz = atan2(-T(1,2), T(1,1));
    fprintf(' > 向量 [X, Y, Z, Rx, Ry, Rz]: [%7.2f, %7.2f, %7.2f, %6.2f°, %6.2f°, %6.2f°]\n', [T(1:3, 4)', rad2deg(rx), rad2deg(ry), rad2deg(rz)]);
    fprintf(' > 矩阵:\n'); disp(T);
end

function R = rotX(rad), R=eye(4); R(2,2)=cos(rad); R(2,3)=-sin(rad); R(3,2)=sin(rad); R(3,3)=cos(rad); end
function R = rotY(rad), R=eye(4); R(1,1)=cos(rad); R(1,3)=sin(rad); R(3,1)=-sin(rad); R(3,3)=cos(rad); end
function R = rotZ(rad), R=eye(4); R(1,1)=cos(rad); R(1,2)=-sin(rad); R(2,1)=sin(rad); R(2,2)=cos(rad); end
function T = trans(x,y,z), T=eye(4); T(1,4)=x; T(2,4)=y; T(3,4)=z; end

function h_surf = drawCylinder(center, vec, r, L, col, alpha_val)
    if nargin < 6, alpha_val = 1.0; end 
    [X, Y, Z] = cylinder(r, 40); Z = Z * L - L/2; 
    vec = vec / norm(vec); v0 = [0, 0, 1]; v = cross(v0, vec); c = dot(v0, vec);
    if norm(v) < 1e-6
        if c > 0, R = eye(3); else, R = diag([1, -1, -1]); end
    else
        vx = [0 -v(3) v(2); v(3) 0 -v(1); -v(2) v(1) 0]; R = eye(3) + vx + vx^2 * ((1 - c) / norm(v)^2);
    end
    for i = 1:numel(X)
        p = R * [X(i); Y(i); Z(i)]; X(i) = p(1)+center(1); Y(i) = p(2)+center(2); Z(i) = p(3)+center(3);
    end
    h_surf = surf(X, Y, Z, 'FaceColor', col, 'EdgeColor', 'none', 'FaceLighting', 'gouraud'); alpha(h_surf, alpha_val);
    h_top = fill3(X(1,:), Y(1,:), Z(1,:), col, 'EdgeColor', 'none'); alpha(h_top, alpha_val);
    h_bot = fill3(X(2,:), Y(2,:), Z(2,:), col, 'EdgeColor', 'none'); alpha(h_bot, alpha_val);
end

function drawFrame(T, scale, name, alpha_val)
    if nargin < 4, alpha_val = 1.0; end 
    O = T(1:3, 4); X = O + T(1:3, 1) * scale; Y = O + T(1:3, 2) * scale; Z = O + T(1:3, 3) * scale;
    drawArrow3D(O, X, [1 0 0], ['X_{', name, '}'], alpha_val);
    drawArrow3D(O, Y, [0 1 0], ['Y_{', name, '}'], alpha_val);
    drawArrow3D(O, Z, [0 0 1], ['Z_{', name, '}'], alpha_val);
end

function drawArrow3D(p1, p2, col, label_text, alpha_val)
    v = p2 - p1; L = norm(v); if L == 0, return; end
    v = v / L; r_shaft = 20; L_shaft = L * 0.85; [X, Y, Z] = cylinder(r_shaft, 20); Z = Z * L_shaft;
    v0 = [0, 0, 1]; c = dot(v0, v); vv = cross(v0, v);
    if norm(vv) < 1e-6
        if c > 0, R = eye(3); else, R = diag([1, -1, -1]); end
    else
        vx = [0 -vv(3) vv(2); vv(3) 0 -vv(1); -vv(2) vv(1) 0]; R = eye(3) + vx + vx^2 * ((1 - c) / norm(vv)^2);
    end
    for i = 1:numel(X)
        p = R * [X(i); Y(i); Z(i)] + p1(1:3); X(i) = p(1); Y(i) = p(2); Z(i) = p(3);
    end
    h1 = surf(X, Y, Z, 'FaceColor', col, 'EdgeColor', 'none', 'FaceLighting', 'gouraud'); alpha(h1, alpha_val);
    r_head = 45; L_head = L - L_shaft; [Xh, Yh, Zh] = cylinder([r_head, 0], 20); Zh = Zh * L_head + L_shaft;
    for i = 1:numel(Xh)
        p = R * [Xh(i); Yh(i); Zh(i)] + p1(1:3); Xh(i) = p(1); Yh(i) = p(2); Zh(i) = p(3);
    end
    h2 = surf(Xh, Yh, Zh, 'FaceColor', col, 'EdgeColor', 'none', 'FaceLighting', 'gouraud'); alpha(h2, alpha_val);
    h3 = fill3(Xh(1,:), Yh(1,:), Zh(1,:), col, 'EdgeColor', 'none'); alpha(h3, alpha_val);
    text(p2(1), p2(2), p2(3), label_text, 'Color', col, 'FontSize', 14, 'FontWeight', 'bold');
end

function drawBox(center, Lx, Ly, Lz, col, alpha_val, edge_col, line_style)
    x = center(1) + [-Lx/2, Lx/2, Lx/2, -Lx/2, -Lx/2, Lx/2, Lx/2, -Lx/2];
    y = center(2) + [-Ly/2, -Ly/2, Ly/2, Ly/2, -Ly/2, -Ly/2, Ly/2, Ly/2];
    z = center(3) + [0, 0, 0, 0, Lz, Lz, Lz, Lz];
    faces = [1 2 6 5; 2 3 7 6; 3 4 8 7; 4 1 5 8; 1 2 3 4; 5 6 7 8];
    patch('Vertices', [x', y', z'], 'Faces', faces, 'FaceColor', col, 'EdgeColor', edge_col, 'LineStyle', line_style, 'FaceAlpha', alpha_val, 'FaceLighting', 'flat');
end

function drawCylinderWireframe(center, vec, r, L, col, line_style, line_width)
    theta = linspace(0, 2*pi, 40); x_circ = r * cos(theta); y_circ = r * sin(theta);
    z_top = L/2 * ones(size(theta)); z_bot = -L/2 * ones(size(theta));
    vec = vec / norm(vec); v0 = [0, 0, 1]; v = cross(v0, vec); c = dot(v0, vec);
    if norm(v) < 1e-6
        if c > 0, R = eye(3); else, R = diag([1, -1, -1]); end
    else
        vx = [0 -v(3) v(2); v(3) 0 -v(1); -v(2) v(1) 0]; R = eye(3) + vx + vx^2 * ((1 - c) / norm(v)^2);
    end
    pts_top = R * [x_circ; y_circ; z_top] + center(:); pts_bot = R * [x_circ; y_circ; z_bot] + center(:);
    plot3(pts_top(1,:), pts_top(2,:), pts_top(3,:), 'Color', col, 'LineStyle', line_style, 'LineWidth', line_width);
    plot3(pts_bot(1,:), pts_bot(2,:), pts_bot(3,:), 'Color', col, 'LineStyle', line_style, 'LineWidth', line_width);
    idx = round(linspace(1, length(theta)-1, 8));
    for j = 1:length(idx)
        k = idx(j);
        plot3([pts_top(1,k), pts_bot(1,k)], [pts_top(2,k), pts_bot(2,k)], [pts_top(3,k), pts_bot(3,k)], ...
              'Color', col, 'LineStyle', line_style, 'LineWidth', line_width);
    end
end