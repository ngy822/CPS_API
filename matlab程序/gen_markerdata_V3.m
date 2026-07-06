% 空间对接系统：激光跟踪仪靶标点模拟数据生成器
clear; clc; close all;

% ================= 1. 机构与工件参数设定 =================
Rw = 3506 / 2;  % 工件半径 (mm)
Lw = 9580;      % 移动段长度
Hc = 1500;      % 顶升柱初始高度
Xc = 2710 / 2;  % 横向跨距一半
rr = 850 / 2;   % 滚筒半径
Zw = Hc + sqrt((Rw + rr)^2 - Xc^2); % 初始高度

% ================= 2. 预设真实的坐标系变换矩阵 (含手动微调) =================
% --- [2.1] 世界系 {W} 与 基座系 {B} ---
T_W = eye(4); 
T_W(2, 4) = Lw/2; % 世界坐标系 {W}

T_B = eye(4); 
T_B(2, 4) = 0;    % 基座坐标系 {B} (初始位于原点)

% --- [2.2] 移动段坐标系 {M0} (坐标系 2) ---
T_M0_base = eye(4);
T_M0_base(2, 4) = Lw/2; 
T_M0_base(3, 4) = Zw;  

% 手动设定：相对于移动段基准位姿的微调量 (模拟移动段的随机停放姿态)
adj_M0_dx = 15.0;   
adj_M0_dy = -25.0;   
adj_M0_dz = 12.0;   
adj_M0_rx = 0.5;    
adj_M0_ry = 0.0;    
adj_M0_rz = -0.8;   

Delta_M0 = trans(adj_M0_dx, adj_M0_dy, adj_M0_dz) * ...
           rotX(deg2rad(adj_M0_rx)) * rotY(deg2rad(adj_M0_ry)) * rotZ(deg2rad(adj_M0_rz));
T_M0 = T_M0_base * Delta_M0;

% --- [2.3] 固定段坐标系 {F} (坐标系 1) ---
T_F_base = eye(4);
T_F_base(2, 4) = Lw/2 + 300;
T_F_base(3, 4) = Zw;

% 手动设定：相对于固定段基准位姿的微调量 (模拟安装固定误差)
adj_F_dx = 150.0;   
adj_F_dy = 0.0;     
adj_F_dz = 100.0;   
adj_F_rx = 1.5;     
adj_F_ry = 3.0;     
adj_F_rz = 0.5;     

Delta_F = trans(adj_F_dx, adj_F_dy, adj_F_dz) * ...
          rotX(deg2rad(adj_F_rx)) * rotY(deg2rad(adj_F_ry)) * rotZ(deg2rad(adj_F_rz));
T_F = T_F_base * Delta_F;

% ================= 3. 在各局部坐标系下生成靶标，并转换到全局 =================
Targets = zeros(18, 3);

% [A] 世界系 {W} (P1, P2, P3)
P_W_local = [0, 2000, 0; 0, 0, 0; 1500, 1000, 0]';
Targets(1:3, :) = (T_W(1:3, 1:3) * P_W_local + T_W(1:3, 4))';

% [B] 基座系 {B} (P4, P5, P6)
P_B_local = [0, 2500, 0; 0, 0, 0; 1800, 1200, 0]';
Targets(4:6, :) = (T_B(1:3, 1:3) * P_B_local + T_B(1:3, 4))';

% [C] 固定段 {F} (P7 ~ P12)
y1_F = 500; y2_F = 1500; 
ang1_F = deg2rad([-30, 0, 30]); ang2_F = deg2rad([-40, 0, 40]); 
P_F_local = [
    Rw*cos(ang1_F(1)), y1_F, Rw*sin(ang1_F(1)); Rw*cos(ang1_F(2)), y1_F, Rw*sin(ang1_F(2)); Rw*cos(ang1_F(3)), y1_F, Rw*sin(ang1_F(3));
    Rw*cos(ang2_F(1)), y2_F, Rw*sin(ang2_F(1)); Rw*cos(ang2_F(2)), y2_F, Rw*sin(ang2_F(2)); Rw*cos(ang2_F(3)), y2_F, Rw*sin(ang2_F(3));
]';
Targets(7:12, :) = (T_F(1:3, 1:3) * P_F_local + T_F(1:3, 4))';

% [D] 移动段 {M0} (P13 ~ P18)
y1_M = -500; y2_M = -1500; 
ang1_M = deg2rad([-25, 10, 45]); ang2_M = deg2rad([-35, 0, 35]); 
P_M_local = [
    Rw*cos(ang1_M(1)), y1_M, Rw*sin(ang1_M(1)); Rw*cos(ang1_M(2)), y1_M, Rw*sin(ang1_M(2)); Rw*cos(ang1_M(3)), y1_M, Rw*sin(ang1_M(3));
    Rw*cos(ang2_M(1)), y2_M, Rw*sin(ang2_M(1)); Rw*cos(ang2_M(2)), y2_M, Rw*sin(ang2_M(2)); Rw*cos(ang2_M(3)), y2_M, Rw*sin(ang2_M(3));
]';
Targets(13:18, :) = (T_M0(1:3, 1:3) * P_M_local + T_M0(1:3, 4))';

% ================= 4. 添加测量噪声与保存数据 =================
noise_level = 0.0; % 测试完毕后可调高此值测试拟合鲁棒性
Targets = Targets + randn(18, 3) * noise_level;

% ★ 保存生成的靶标数据为 .mat 文件，供逆解程序读取
save('laser_tracker_data.mat', 'Targets');
disp('>> 成功：靶标点云数据已保存至 laser_tracker_data.mat');
disp(' ');

% ★ 新增：将靶标数据按照指定的 CSV 格式保存
csv_filename = 'measurements.csv';
fid = fopen(csv_filename, 'w');
% 写入标准表头
fprintf(fid, 'Index,X,Y,Z,TargetSize,Timestamp\n');
% 模拟一个统一的时间戳
dummy_time = datestr(now, 'yyyy-mm-dd HH:MM:SS');
% 循环写入 18 个靶标点的数据
for i = 1:18
    % TargetSize 统一使用占位符 1.5
    fprintf(fid, '%d,%.3f,%.3f,%.3f,1.5,%s\n', i, Targets(i,1), Targets(i,2), Targets(i,3), dummy_time);
end
fclose(fid);
disp(['>> 成功：靶标点云数据已导出为CSV格式至 ', csv_filename]);
disp(' ');

% ================= 5. 数据输出与打印 (已恢复) =================
disp('==================== 靶标模拟数据生成配置 ====================');
fprintf('移动段 {M0} 注入微调: [%.2f, %.2f, %.2f] mm | [%.2f, %.2f, %.2f] 度\n', ...
    adj_M0_dx, adj_M0_dy, adj_M0_dz, adj_M0_rx, adj_M0_ry, adj_M0_rz);
fprintf('固定段 {F}  注入微调: [%.2f, %.2f, %.2f] mm | [%.2f, %.2f, %.2f] 度\n\n', ...
    adj_F_dx, adj_F_dy, adj_F_dz, adj_F_rx, adj_F_ry, adj_F_rz);

disp('================ 激光跟踪仪靶标点模拟数据 (全局绝对坐标 mm) ================');
fprintf('%-5s %-12s %-12s %-12s | %s\n', '点号', 'X', 'Y', 'Z', '归属与说明');
fprintf('----------------------------------------------------------------------\n');
descriptions = {
    '世界系 {W} (决定Y轴)'; 
    '世界系 {W} (原点)'; 
    '世界系 {W} (辅助Z轴)';
    
    '基座系 {B} (决定Y轴)'; 
    '基座系 {B} (原点)'; 
    '基座系 {B} (辅助Z轴)';
    
    '固定段 {F} (近端排 1)'; 
    '固定段 {F} (近端排 2)'; 
    '固定段 {F} (近端排 3)';
    
    '固定段 {F} (远端排 1)'; 
    '固定段 {F} (远端排 2 -> 决定X轴)'; 
    '固定段 {F} (远端排 3)';
    
    '移动段 {M} (近端排 1)'; 
    '移动段 {M} (近端排 2)'; 
    '移动段 {M} (近端排 3)';
    
    '移动段 {M} (远端排 1)'; 
    '移动段 {M} (远端排 2 -> 决定X轴)'; 
    '移动段 {M} (远端排 3)'
};

for i = 1:18
    fprintf('P%-4d %12.3f %12.3f %12.3f | %s\n', i, Targets(i,1), Targets(i,2), Targets(i,3), descriptions{i});
end
disp('======================================================================');

% ================= 6. 3D 可视化检验 (已恢复) =================
figure('Name', '激光跟踪仪靶标点云', 'Color', 'w', 'Position', [100, 100, 1000, 800]);
hold on; grid on; axis equal; view(-135, 30);
xlabel('X 轴'); ylabel('Y 轴'); zlabel('Z 轴');
title('空间靶标点分布 (红:地基, 绿:移动段, 蓝:固定段)', 'FontSize', 14);

% 绘制靶标点
scatter3(Targets(1:6, 1), Targets(1:6, 2), Targets(1:6, 3), 80, 'r', 'filled');    
scatter3(Targets(7:12, 1), Targets(7:12, 2), Targets(7:12, 3), 80, 'b', 'filled'); 
scatter3(Targets(13:18, 1), Targets(13:18, 2), Targets(13:18, 3), 80, 'g', 'filled'); 

% 标注点编号
for i = 1:18
    text(Targets(i,1)+100, Targets(i,2), Targets(i,3)+100, sprintf('P%d', i), 'FontSize', 10, 'FontWeight', 'bold');
end

% 绘制辅助建系连线
plot3(Targets([2,1],1), Targets([2,1],2), Targets([2,1],3), 'r--', 'LineWidth', 1.5); 
plot3(Targets([5,4],1), Targets([5,4],2), Targets([5,4],3), 'r--', 'LineWidth', 1.5); 
plot3(Targets([10,12],1), Targets([10,12],2), Targets([10,12],3), 'b:'); 
plot3(Targets([16,18],1), Targets([16,18],2), Targets([16,18],3), 'g:'); 

% 绘制移动段和固定段局部坐标系的三向坐标轴，直观查看微调偏转
scale = 1200;
O_F = T_F(1:3, 4); X_F = O_F + T_F(1:3, 1) * scale; Y_F = O_F + T_F(1:3, 2) * scale; Z_F = O_F + T_F(1:3, 3) * scale;
plot3([O_F(1) X_F(1)], [O_F(2) X_F(2)], [O_F(3) X_F(3)], 'r-', 'LineWidth', 2);
plot3([O_F(1) Y_F(1)], [O_F(2) Y_F(2)], [O_F(3) Y_F(3)], 'g-', 'LineWidth', 2);
plot3([O_F(1) Z_F(1)], [O_F(2) Z_F(2)], [O_F(3) Z_F(3)], 'b-', 'LineWidth', 2);

O_M = T_M0(1:3, 4); X_M = O_M + T_M0(1:3, 1) * scale; Y_M = O_M + T_M0(1:3, 2) * scale; Z_M = O_M + T_M0(1:3, 3) * scale;
plot3([O_M(1) X_M(1)], [O_M(2) X_M(2)], [O_M(3) X_M(3)], 'r-', 'LineWidth', 2);
plot3([O_M(1) Y_M(1)], [O_M(2) Y_M(2)], [O_M(3) Y_M(3)], 'g-', 'LineWidth', 2);
plot3([O_M(1) Z_M(1)], [O_M(2) Z_M(2)], [O_M(3) Z_M(3)], 'b-', 'LineWidth', 2);

axis([-4000 4000 -4000 10000 -1000 6000]);

% --- 辅助旋转矩阵函数 ---
function R = rotX(rad), R=eye(4); R(2,2)=cos(rad); R(2,3)=-sin(rad); R(3,2)=sin(rad); R(3,3)=cos(rad); end
function R = rotY(rad), R=eye(4); R(1,1)=cos(rad); R(1,3)=sin(rad); R(3,1)=-sin(rad); R(3,3)=cos(rad); end
function R = rotZ(rad), R=eye(4); R(1,1)=cos(rad); R(1,2)=-sin(rad); R(2,1)=sin(rad); R(2,2)=cos(rad); end
function T = trans(x,y,z), T=eye(4); T(1,4)=x; T(2,4)=y; T(3,4)=z; end