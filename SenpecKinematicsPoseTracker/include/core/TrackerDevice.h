#pragma once

#include <atomic>
#include <string>

#include "api/LaserTrackerTypes.h"
#include "api/LaserTrackerInterface.h"

namespace senpec
{
    struct MeasurementPoint
    {
        double x{ 0.0 };
        double y{ 0.0 };
        double z{ 0.0 };
    };

    struct TrackerRealTimeStatus
    {
        double azimuth{ 0.0 };
        double elevation{ 0.0 };
        double x{ 0.0 };
        double y{ 0.0 };
        double z{ 0.0 };
        double distance{ 0.0 };
        double intensity{ 0.0 };
        bool isLaserPathError{ false };
        bool isWarmingUp{ false };
        double temperature{ 0.0 };
        double pressure{ 0.0 };
        double humidity{ 0.0 };
        api::lt::OperationMode operationMode{ api::lt::OperationMode::Idle };
    };

    class TrackerDevice
    {
    public:
        TrackerDevice();
        ~TrackerDevice() = default;

        void StartAutoDetect();
        void StartConnectAsync(const std::string& ipAddress);
        void StartDisconnectAsync();

        std::atomic<bool> isSearching_{ false };
        std::atomic<bool> detectCompleted_{ false };
        bool detectSuccess_{ false };
        std::string detectedIp_;

        std::atomic<bool> isConnecting_{ false };
        std::atomic<bool> connectCompleted_{ false };
        bool connectSuccess_{ false };

        std::atomic<bool> isDisconnecting_{ false };
        std::atomic<bool> disconnectCompleted_{ false };

        void StartMeasureAsync();
        std::atomic<bool> isMeasuring_{ false };
        std::atomic<bool> measureCompleted_{ false };
        bool measureSuccess_{ false };

        MeasurementPoint lastMeasurePoint_{};
        double lastMeasureAvg_{ 0.0 };
        double lastMeasureMax_{ 0.0 };
        double lastMeasureRms_{ 0.0 };

        void StartSearchTargetAsync();
        std::atomic<bool> isSearchingTarget_{ false };

        bool IsConnected() const;
        api::lt::ErrorType SwitchToIdle();
        api::lt::ErrorType SwitchToPosition();
        api::lt::ErrorType SwitchToTrack();
        api::lt::ErrorType EnableCameraSearch(bool enable);

        api::lt::ErrorType Home(api::lt::SmrSize smr_size);

        void MoveAzimuth(double stepDegrees);
        void MoveElevation(double stepDegrees);
        api::lt::ErrorType StopMotionByModeChange();
        void StopMotion();

        TrackerRealTimeStatus GetRealTimeStatus() const;

    private:
        static void TaskCallback(const api::lt::TaskResult& taskStatus, void* context);

        std::atomic<bool> realTimeConnected_{ false };
        mutable api::lt::LaserTrackerInterface apiDevice_;
    };
}
