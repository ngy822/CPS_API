#include "core/AlignmentSolver.h"
#include "core/OperationLogger.h"

// 【修复 MSVC 中文及特殊符号截断神坑】
#pragma execution_character_set("utf-8")

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem> // 【新增】引入文件系统库处理中文路径

// 【修复 MSVC 下 M_PI 未定义的神坑】
#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <iomanip>

#define DEG2RAD(x) ((x) * M_PI / 180.0)
#define RAD2DEG(x) ((x) * 180.0 / M_PI)

namespace senpec
{
    using namespace Eigen;
    using namespace std;

    AlignmentSolver::AlignmentSolver()
    {
        Rw = 3506.0 / 2.0;
        Lw = 9580.0;
        L_fixed = 4000.0;
        Xc = 2710.0 / 2.0;
        Yc = 4500.0 / 2.0;
        Hc = 1500.0;
        rr = 850.0 / 2.0;
        Lr = 350.0;
        Zw = Hc + sqrt(pow(Rw + rr, 2) - pow(Xc, 2));

        T_W_ideal = Matrix4d::Identity();
        T_W_ideal(1, 3) = Lw / 2.0;

        man_dx = 0; man_dy = 0; man_dz = 0;

        man_rx = 0; man_ry = 0; man_rz = 0;

        H_cols_cmd.assign(4, 0.0);
        vec_M0.assign(6, 0.0);
        vec_F.assign(6, 0.0);
        vec_Mnew.assign(6, 0.0);
    }

    void AlignmentSolver::setManualAdjustments(double dx, double dy, double dz, double rx, double ry, double rz)
    {
        man_dx = dx; man_dy = dy; man_dz = dz;
        man_rx = rx; man_ry = ry; man_rz = rz;
    }

    Matrix4d AlignmentSolver::trans(double x, double y, double z)
    {
        Matrix4d T = Matrix4d::Identity();
        T(0, 3) = x; T(1, 3) = y; T(2, 3) = z;
        return T;
    }

    Matrix4d AlignmentSolver::rotX(double rad)
    {
        Matrix4d R = Matrix4d::Identity();
        R(1, 1) = cos(rad); R(1, 2) = -sin(rad);
        R(2, 1) = sin(rad); R(2, 2) = cos(rad);
        return R;
    }

    Matrix4d AlignmentSolver::rotY(double rad)
    {
        Matrix4d R = Matrix4d::Identity();
        R(0, 0) = cos(rad); R(0, 2) = sin(rad);
        R(2, 0) = -sin(rad); R(2, 2) = cos(rad);
        return R;
    }

    Matrix4d AlignmentSolver::rotZ(double rad)
    {
        Matrix4d R = Matrix4d::Identity();
        R(0, 0) = cos(rad); R(0, 1) = -sin(rad);
        R(1, 0) = sin(rad); R(1, 1) = cos(rad);
        return R;
    }

    Vector3d AlignmentSolver::fitCircle3D(const Vector3d& p1, const Vector3d& p2, const Vector3d& p3)
    {
        Vector3d v1 = p2 - p1;
        Vector3d v2 = p3 - p1;
        Vector3d n = v1.cross(v2);
        Vector3d cross_part = (v1.squaredNorm() * v2 - v2.squaredNorm() * v1).cross(n);
        return p1 + cross_part / (2.0 * n.squaredNorm());
    }

    vector<double> AlignmentSolver::extractPoseVector(const Matrix4d& T)
    {
        double ry = asin(T(0, 2));
        double rx = atan2(-T(1, 2), T(2, 2));
        double rz = atan2(-T(0, 1), T(0, 0));
        return { T(0, 3), T(1, 3), T(2, 3), RAD2DEG(rx), RAD2DEG(ry), RAD2DEG(rz) };
    }

    void AlignmentSolver::printPose(const string& title, const Matrix4d& T, const vector<double>& vec)
    {
        ostringstream oss;
        oss << u8"【" << title << u8"】\n";
        oss << u8" > 向量 [X, Y, Z, Rx, Ry, Rz]: "
            << "[" << fixed << setprecision(2) << vec[0] << ", " << vec[1] << ", " << vec[2] << ", "
            << vec[3] << u8"°, " << vec[4] << u8"°, " << vec[5] << u8"°]";
        OperationLogger::Instance().Log(LogLevel::Info, oss.str());
    }

    bool AlignmentSolver::parseTargetsCSV(const string& filename, vector<Vector3d>& targets)
    {
        std::filesystem::path filePath = std::filesystem::u8path(filename);
        ifstream file(filePath);
        if (!file.is_open()) {
            last_error_message = u8"无法打开靶标文件，请检查导入路径。";
            OperationLogger::Instance().Log(LogLevel::Error, u8"无法打开靶标文件: " + filename);
            return false;
        }

        string line;
        getline(file, line);

        targets.clear();
        int lineIndex = 2; // 用于在报错时提示错在哪一行
        while (getline(file, line)) {
            if (line.empty() || line == "\r") { lineIndex++; continue; }

            stringstream ss(line);
            string token;
            vector<string> cols;

            while (getline(ss, token, ',')) {
                cols.push_back(token);
            }

            if (cols.size() >= 4) {
                try {
                    double x = stod(cols[1]);
                    double y = stod(cols[2]);
                    double z = stod(cols[3]);
                    targets.push_back(Vector3d(x, y, z));
                }
                catch (...) {
                    last_error_message = u8"CSV数据解析异常 (出现在第 " + to_string(lineIndex) + u8" 行)";
                    OperationLogger::Instance().Log(LogLevel::Error, u8"数据解析异常，行内容: " + line);
                    return false;
                }
            }
            lineIndex++;
        }

        if (targets.size() < 18) {
            last_error_message = u8"靶标点数量不足 18 个 (当前仅提取到 " + to_string(targets.size()) + u8" 个有效点)";
            OperationLogger::Instance().Log(LogLevel::Error, u8"靶标点数量不足 18 个，当前读取数量：" + to_string(targets.size()));
            return false;
        }
        return true;
    }

    void AlignmentSolver::computeAlignment(const string& csvFile, int align_mode)
    {
        is_success = false;
        last_error_message.clear(); // 每次执行计算前，先清空历史错误信息

        vector<Vector3d> targets;
        if (!parseTargetsCSV(csvFile, targets)) {
            return;
        }

        // ================= 1. 拟合世界系 {W_fitted} =================
        Vector3d p1 = targets[0], p2 = targets[1], p3 = targets[2];
        Vector3d O_W_f = p2;
        Vector3d Y_W_f = (p1 - p2).normalized();
        Vector3d Z_W_f = (p3 - p2).cross(p1 - p2).normalized();
        Vector3d X_W_f = Y_W_f.cross(Z_W_f).normalized();
        Matrix4d T_W_fitted = Matrix4d::Identity();
        T_W_fitted.block<3, 1>(0, 0) = X_W_f;
        T_W_fitted.block<3, 1>(0, 1) = Y_W_f;
        T_W_fitted.block<3, 1>(0, 2) = Z_W_f;
        T_W_fitted.block<3, 1>(0, 3) = O_W_f;

        // ================= 2. 拟合固定段端面坐标系 {F_fitted} =================
        Vector3d C1_F = fitCircle3D(targets[6], targets[7], targets[8]);
        Vector3d C2_F = fitCircle3D(targets[9], targets[10], targets[11]);
        Vector3d Y_F_f = (C2_F - C1_F).normalized();
        Vector3d O_F_f = C1_F - Y_F_f * 500.0;
        Vector3d p11 = targets[10];
        Vector3d X_F_temp = (p11 - C2_F).normalized();
        Vector3d X_F_f = (X_F_temp - X_F_temp.dot(Y_F_f) * Y_F_f).normalized();
        Vector3d Z_F_f = X_F_f.cross(Y_F_f);
        Matrix4d T_F_fitted = Matrix4d::Identity();
        T_F_fitted.block<3, 1>(0, 0) = X_F_f;
        T_F_fitted.block<3, 1>(0, 1) = Y_F_f;
        T_F_fitted.block<3, 1>(0, 2) = Z_F_f;
        T_F_fitted.block<3, 1>(0, 3) = O_F_f;

        // ================= 3. 拟合移动段端面坐标系 {M0_fitted} =================
        Vector3d C1_M = fitCircle3D(targets[12], targets[13], targets[14]);
        Vector3d C2_M = fitCircle3D(targets[15], targets[16], targets[17]);
        Vector3d Y_M_f = (C1_M - C2_M).normalized();
        Vector3d O_M_f = C1_M + Y_M_f * 500.0;
        Vector3d p17 = targets[16];
        Vector3d X_M_temp = (p17 - C2_M).normalized();
        Vector3d X_M_f = (X_M_temp - X_M_temp.dot(Y_M_f) * Y_M_f).normalized();
        Vector3d Z_M_f = X_M_f.cross(Y_M_f);
        Matrix4d T_M0_fitted = Matrix4d::Identity();
        T_M0_fitted.block<3, 1>(0, 0) = X_M_f;
        T_M0_fitted.block<3, 1>(0, 1) = Y_M_f;
        T_M0_fitted.block<3, 1>(0, 2) = Z_M_f;
        T_M0_fitted.block<3, 1>(0, 3) = O_M_f;

        // ================= 4. 空间映射转换 =================
        Matrix4d T_LT2Machine = T_W_ideal * T_W_fitted.inverse();
        Matrix4d T_M0 = T_LT2Machine * T_M0_fitted;
        Matrix4d T_F_curr = T_LT2Machine * T_F_fitted;

        // ================= 5. 对接匹配策略生成 =================
        Matrix4d T_Mnew;
        string mode_str;

        if (align_mode == 1) {
            T_Mnew = T_F_curr;
            mode_str = u8"[模式 1] 完全重合 (目标锁定固定段端面)";
        }
        else if (align_mode == 2) {
            T_Mnew = T_F_curr;
            T_Mnew.block<3, 1>(0, 3) = T_M0.block<3, 1>(0, 3);
            mode_str = u8"[模式 2] 仅姿态平行 (保留原地位置)";
        }
        else if (align_mode == 3) {
            Matrix4d T_manual = trans(man_dx, man_dy, man_dz) * rotX(DEG2RAD(man_rx)) * rotY(DEG2RAD(man_ry)) * rotZ(DEG2RAD(man_rz));
            T_Mnew = T_M0 * T_manual;
            mode_str = u8"[模式 3] 人工手动设定调整量";
        }
        else {
            last_error_message = u8"未知的匹配模式";
            OperationLogger::Instance().Log(LogLevel::Error, u8"未知的匹配模式！");
            return;
        }

        Vector3d P_a = T_Mnew.block<3, 1>(0, 3);
        Vector3d v_a = T_Mnew.block<3, 1>(0, 1);

        // ================= 6. 逆向运动学求根 (IK) =================
        vector<double> X_cols = { Xc, Xc, -Xc, -Xc };
        vector<double> Y_cols = { Yc, -Yc, Yc, -Yc };

        dy_cmd = T_Mnew(1, 3) - T_M0(1, 3);
        vector<double> Y_cols_new = { Yc + dy_cmd, -Yc + dy_cmd, Yc + dy_cmd, -Yc + dy_cmd };

        Matrix4d Delta_T_M = T_M0.inverse() * T_Mnew;
        double target_roll_M = RAD2DEG(asin(Delta_T_M(0, 2)));
        roll_cmd_fixed = -target_roll_M;

        for (int i = 0; i < 4; i++) {
            double t_i = (Y_cols_new[i] - P_a(1)) / v_a(1);
            double X_axis_i = P_a(0) + t_i * v_a(0);
            double Z_axis_i = P_a(2) + t_i * v_a(2);

            double inner_val = pow(Rw + rr, 2) - pow(X_cols[i] - X_axis_i, 2);
            if (inner_val < 0) {
                last_error_message = u8"计算出的动作幅度超限！支链 " + to_string(i + 1) + u8" 存在脱轨风险。";
                OperationLogger::Instance().Log(LogLevel::Error, u8"幅度超限！支链 " + to_string(i + 1) + u8" 脱轨！");
                return;
            }
            H_cols_cmd[i] = Z_axis_i - sqrt(inner_val);
        }

        // ================= 7. 筒体对接质量评估计算 =================
        Vector3d Y_axis_M0 = T_M0.block<3, 1>(0, 1);
        Vector3d Y_axis_F = T_F_curr.block<3, 1>(0, 1);

        double dot_val = std::clamp(Y_axis_M0.dot(Y_axis_F), -1.0, 1.0);
        axis_angle_dev = RAD2DEG(acos(dot_val));

        Vector3d center_M0 = T_M0.block<3, 1>(0, 3);
        Vector3d center_F = T_F_curr.block<3, 1>(0, 3);
        Vector3d dP = center_F - center_M0;

        end_face_spacing = std::abs(dP.dot(Y_axis_F));

        Vector3d dP_proj = dP - dP.dot(Y_axis_F) * Y_axis_F;
        misalignment = dP_proj.norm();

        // ================= 8. 数据存储与日志输出 =================
        mat_M0 = T_W_ideal.inverse() * T_M0;
        mat_F = T_W_ideal.inverse() * T_F_curr;
        mat_Mnew = T_W_ideal.inverse() * T_Mnew;

        vec_M0 = extractPoseVector(mat_M0);
        vec_F = extractPoseVector(mat_F);
        vec_Mnew = extractPoseVector(mat_Mnew);

        OperationLogger::Instance().Log(LogLevel::Info, u8"================ 激光跟踪仪测点拟合输出 ================");
        printPose(u8"坐标系 2 (实测移动段当前端面 M0)", mat_M0, vec_M0);
        printPose(u8"坐标系 1 (实测固定段目标端面 F)", mat_F, vec_F);
        printPose(u8">> 最终选定的移动段目标位姿 (Mnew)", mat_Mnew, vec_Mnew);
        OperationLogger::Instance().Log(LogLevel::Info, u8">> 激活对接策略: " + mode_str);

        is_success = true;
    }
}