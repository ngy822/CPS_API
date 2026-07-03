#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>

namespace senpec
{
    // =====================================================================
    // 并联调姿与对接系统底层解算器
    // =====================================================================
    class AlignmentSolver
    {
    public:
        AlignmentSolver();
        ~AlignmentSolver() = default;

        // 设置模式3 (人工调整) 时的微调参数
        void setManualAdjustments(double dx, double dy, double dz, double rx, double ry, double rz);

        // 核心解算入口 (读取 CSV，生成控制指令)
        void computeAlignment(const std::string& csvFile, int align_mode);

        // =====================================================================
        // 对接与调姿计算结果 (公开成员变量，供 UI 直接读取)
        // =====================================================================

        bool is_success{ false }; // 标记最近一次解算是否成功

        // --- 1. 目标位姿相关数据 ---
        Eigen::Matrix4d mat_M0;
        std::vector<double> vec_M0;

        Eigen::Matrix4d mat_F;
        std::vector<double> vec_F;

        Eigen::Matrix4d mat_Mnew;
        std::vector<double> vec_Mnew;

        // --- 2. 分解运动控制指令 (6-DOF) ---
        double roll_cmd_fixed{ 0.0 };
        double dy_cmd{ 0.0 };
        std::vector<double> H_cols_cmd;

        // --- 3. 筒体对接质量评估 (新补齐的自动计算指标) ---
        double axis_angle_dev{ 0.0 };   // 轴线夹角偏差 (°)
        double end_face_spacing{ 0.0 }; // 端面间距 (mm)
        double misalignment{ 0.0 };     // 错边量 (mm)

        // 【新增】用于对外输出具体的错误原因，以便在界面日志中展示
        std::string last_error_message;

    private:
        // 机构与工件参数
        double Rw, Lw, L_fixed;
        double Xc, Yc, Hc, rr, Lr, Zw;
        Eigen::Matrix4d T_W_ideal;

        // 手动微调参数
        double man_dx, man_dy, man_dz, man_rx, man_ry, man_rz;

        // 内部辅助数学函数
        Eigen::Matrix4d trans(double x, double y, double z);
        Eigen::Matrix4d rotX(double rad);
        Eigen::Matrix4d rotY(double rad);
        Eigen::Matrix4d rotZ(double rad);
        Eigen::Vector3d fitCircle3D(const Eigen::Vector3d& p1, const Eigen::Vector3d& p2, const Eigen::Vector3d& p3);
        std::vector<double> extractPoseVector(const Eigen::Matrix4d& T);
        void printPose(const std::string& title, const Eigen::Matrix4d& T, const std::vector<double>& vec);
        bool parseTargetsCSV(const std::string& filename, std::vector<Eigen::Vector3d>& targets);
    };
}