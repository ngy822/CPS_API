#pragma once

#include <array>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "core/TrackerDevice.h"
#include "core/MeasurementStore.h"
#include "core/AlignmentSolver.h" 
#include "core/DigitalTwinPublisher.h"

namespace senpec
{
    enum class MotionMode
    {
        Idle = 0,
        Position = 1,
        Track = 2,
    };

    struct SimulatedKinematicsResult
    {
        double wheel1Height{ 0.0 };
        double wheel2Height{ 0.0 };
        double wheel3Height{ 0.0 };
        double wheel4Height{ 0.0 };
        double rollAngle{ 0.0 };
        double forwardDistance{ 0.0 };

        std::array<double, 16> currentPoseMatrix{};
        std::array<double, 16> targetPoseMatrix{};

        // Current pose vector in 6-DOF form (X, Y, Z, Rx, Ry, Rz)
        std::array<double, 6> currentPoseVector{};
        std::array<double, 6> targetPoseVector{};

        double axisAngleDeviation{ 0.0 };
        double endFaceSpacing{ 0.0 };
        double misalignment{ 0.0 };
    };

    struct RoundRecord
    {
        int roundIndex;
        std::string timestamp;
        std::vector<MeasurementRecord> points;
        SimulatedKinematicsResult kinematics;
    };

    class UiLayer
    {
    public:
        UiLayer(TrackerDevice& device, MeasurementStore& store, AlignmentSolver& solver, DigitalTwinPublisher& publisher);
        ~UiLayer();

        void Render();

        float GetUiScale() const;
        float GetTargetFps() const;
        const std::array<float, 4>& GetBackgroundColor() const;
        const std::array<float, 4>& GetWidgetColor() const;
        const std::array<float, 4>& GetTextColor() const;

        const std::string& GetLogFilePath() const;
        bool OpenLogFile() const;

        bool HasCustomWidgetColor() const;
        bool HasCustomTextColor() const;

        void SetThemeDefaults(const std::array<float, 4>& backgroundColor, const std::array<float, 4>& widgetColor, const std::array<float, 4>& textColor);

    private:
        void RenderMainMenu();
        void RenderMainWorkspace();
        void RenderConnectionPanel();
        void RenderTargetPanel();
        void RenderControlPanel();
        void RenderTablePanel();
        void RenderDisplaySettingsWindow();
        void RenderAboutWindow();
        void RenderKinematicsWindow();
        void RenderRuntimeLogWindow();
        void RenderMatrix4x4(const char* label, const std::array<double, 16>& matrix);
        // Render the 6-DOF pose vector (3 translation + 3 rotation)
        void RenderPoseVector(const char* label, const std::array<double, 6>& vec);
        void RenderSplitterWindows(float& leftWidth, float& topHeight);

        void HandleMeasurement();
        void AppendRuntimeLog(const std::string& line);
        void RestoreDefaultDisplaySettings();

        void StartKinematicsSolve();

        void ExportDockingReport(const std::string& path);
        void RenderUserManualWindow();
        void OpenSystemFile(const std::string& pathStr, const std::string& fileDesc);

        TrackerDevice& device_;
        MeasurementStore& store_;
        AlignmentSolver& solver_;
        DigitalTwinPublisher& publisher_;

        std::string ipAddress_;
        std::string exportPath_;
        int targetSizeIndex_;

        TrackerRealTimeStatus cachedStatus_{};

        double simulatedAzimuth_{ 0.0 };
        double simulatedElevation_{ 0.0 };

        float uiScale_;
        float targetFps_;
        std::array<float, 4> backgroundColor_;
        std::array<float, 4> defaultBackgroundColor_;
        std::array<float, 4> widgetColor_;
        std::array<float, 4> defaultWidgetColor_;
        std::array<float, 4> textColor_;
        std::array<float, 4> defaultTextColor_;

        float defaultUiScale_;
        float defaultTargetFps_;

        bool useCustomBackgroundColor_;
        bool useCustomWidgetColor_;
        bool useCustomTextColor_;
        bool enableCameraSearch_;

        bool showDisplaySettings_;
        bool showAboutWindow_;
        bool showUserManualWindow_;

        bool requestMeasure_;

        std::string kinematicsCsvPath_;
        SimulatedKinematicsResult simulatedKinematicsResult_;
        bool hasSimulatedKinematicsResult_;

        float leftPaneRatio_;
        float topPaneRatio_;

        std::string runtimeLog_;
        bool autoScrollLog_;
        bool scrollToBottom_;

        MotionMode currentMotionMode_{ MotionMode::Track };

        bool isSimulationMode_;
        bool simulatedConnected_;

        bool isDetectingIp_;
        bool detectCompleted_;
        bool detectSuccess_;
        std::mutex detectMutex_;
        std::thread detectThread_;

        int currentRoundIndex_{ 1 };
        size_t lastSavedPointCount_{ 0 };
        std::vector<RoundRecord> historyRounds_;
        std::string reportExportPath_;

        unsigned int logoTextureId_{ 0 };
        int logoWidth_{ 0 };
        int logoHeight_{ 0 };
        bool logoLoadAttempted_{ false };

        // Texture resource for the user manual image and its dimensions
        unsigned int manualTextureId_{ 0 };
        int manualWidth_{ 0 };
        int manualHeight_{ 0 };
        bool manualLoadAttempted_{ false };
    };
}