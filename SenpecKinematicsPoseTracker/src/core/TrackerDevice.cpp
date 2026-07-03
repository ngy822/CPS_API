#include "core/TrackerDevice.h"
#include "core/OperationLogger.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace senpec
{
    namespace
    {
        using api::lt::ErrorType;
        using api::lt::TaskMode;

        std::wstring GetExecutableDirectory()
        {
            std::wstring buffer(MAX_PATH, L'\0');
            const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (length == 0)
            {
                return std::filesystem::current_path().wstring();
            }

            buffer.resize(length);
            return std::filesystem::path(buffer).parent_path().wstring();
        }

        std::string WideToUtf8(const std::wstring& text)
        {
            if (text.empty())
            {
                return {};
            }

            const int required = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (required <= 0)
            {
                return std::string(text.begin(), text.end());
            }

            std::string utf8(static_cast<size_t>(required - 1), '\0');
            WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, utf8.data(), required, nullptr, nullptr);
            return utf8;
        }

        std::string DescribeApiError(ErrorType code)
        {
            switch (code)
            {
            case ErrorType::Success: return "Success";
            case ErrorType::OtherTaskOnGoing: return "OtherTaskOnGoing";
            case ErrorType::InvalidCallbackParameter: return "InvalidCallbackParameter";
            case ErrorType::DeviceAlreadyConnected: return "DeviceAlreadyConnected";
            case ErrorType::TaskOperationTimeout: return "TaskOperationTimeout";
            case ErrorType::DataNotUpdating: return "DataNotUpdating";
            case ErrorType::OtherMeasurementRunning: return "OtherMeasurementRunning";
            case ErrorType::NoMeasurementIsRunning: return "NoMeasurementIsRunning";
            case ErrorType::FailedToDiscoverIpAddress: return "FailedToDiscoverIpAddress";
            case ErrorType::FailedToConnectToFirmware: return "FailedToConnectToFirmware";
            case ErrorType::FailedToConnectToCamera: return "FailedToConnectToCamera";
            case ErrorType::DeviceNotConnected: return "DeviceNotConnected";
            case ErrorType::FailedToConnectToRealTimeDataPort: return "FailedToConnectToRealTimeDataPort";
            case ErrorType::CommunicationFailed: return "CommunicationFailed";
            case ErrorType::InvalidInputParameter: return "InvalidInputParameter";
            case ErrorType::InternalError: return "InternalError";
            case ErrorType::TaskAborted: return "TaskAborted";
            case ErrorType::JoggingLimitReached: return "JoggingLimitReached";
            case ErrorType::Unsupported: return "Unsupported";
            case ErrorType::LevelAngleOutOfRange: return "LevelAngleOutOfRange";
            case ErrorType::MeasurementNotValid: return "MeasurementNotValid";
            case ErrorType::NoSmrAtHomePosition: return "NoSmrAtHomePosition";
            case ErrorType::TrackerNotWarmedUp: return "TrackerNotWarmedUp";
            case ErrorType::AdmLowIntensity: return "AdmLowIntensity";
            case ErrorType::TargetNotTracking: return "TargetNotTracking";
            case ErrorType::HomingFailure: return "HomingFailure";
            case ErrorType::MultiSmrMeasurementFinished: return "MultiSmrMeasurementFinished";
            case ErrorType::FTPManagerCommInitializationFailed: return "FTPManagerCommInitializationFailed";
            case ErrorType::FTPManagerInvalidIPAddress: return "FTPManagerInvalidIPAddress";
            case ErrorType::FTPManagerInvalidLocalDirectory: return "FTPManagerInvalidLocalDirectory";
            case ErrorType::FTPManagerInvalidRemoteDirectory: return "FTPManagerInvalidRemoteDirectory";
            case ErrorType::FTPManagerInvalidRemoteSession: return "FTPManagerInvalidRemoteSession";
            case ErrorType::FTPManagerErrorPerformingFTPCommands: return "FTPManagerErrorPerformingFTPCommands";
            case ErrorType::FTPManagerInvalidFileName: return "FTPManagerInvalidFileName";
            case ErrorType::FTPManagerRemoteAndLocalFileSizeMismatch: return "FTPManagerRemoteAndLocalFileSizeMismatch";
            case ErrorType::FTPManagerCouldNotOpenFileForWriting: return "FTPManagerCouldNotOpenFileForWriting";
            case ErrorType::AccessoryPrmFileWrongFormat: return "AccessoryPrmFileWrongFormat";
            case ErrorType::AccessoryPrmFileNotFound: return "AccessoryPrmFileNotFound";
            case ErrorType::PRMFileIncorrectFormat: return "PRMFileIncorrectFormat";
            case ErrorType::PRMFileNotRead: return "PRMFileNotRead";
            case ErrorType::DeviceNotSupported: return "DeviceNotSupported";
            case ErrorType::RadiusLimitReached: return "RadiusLimitReached";
            case ErrorType::TimedoutNotificationNotReceived: return "TimedoutNotificationNotReceived";
            case ErrorType::IndexSearchFailed: return "IndexSearchFailed";
            case ErrorType::TargetIsNotStable: return "TargetIsNotStable";
            case ErrorType::IvisionWindowNotInitialized: return "IvisionWindowNotInitialized";
            case ErrorType::NoProbingDeviceDetected: return "NoProbingDeviceDetected";
            case ErrorType::NoDataIsCollected: return "NoDataIsCollected";
            case ErrorType::No6DDeviceDetected: return "No6DDeviceDetected";
            case ErrorType::VirtualLevelNotPerformed: return "VirtualLevelNotPerformed";
            case ErrorType::SmrIsNotLockedOn: return "SmrIsNotLockedOn";
            case ErrorType::AccessoryConnected: return "AccessoryConnected";
            case ErrorType::VirtualLevelNotEnabled: return "VirtualLevelNotEnabled";
            case ErrorType::LevelSensorIsNotPresent: return "LevelSensorIsNotPresent";
            case ErrorType::Iscan3dAppNotFound: return "Iscan3dAppNotFound";
            case ErrorType::FailedToStartIscan3dApp: return "FailedToStartIscan3dApp";
            case ErrorType::Iscan3dCameraCalibrationFileNotFound: return "Iscan3dCameraCalibrationFileNotFound";
            case ErrorType::Iscan3dCalibrationFileNotFound: return "Iscan3dCalibrationFileNotFound";
            case ErrorType::FailedToConnectToIscan3dApp: return "FailedToConnectToIscan3dApp";
            case ErrorType::FailedToStartIs3dScanning: return "FailedToStartIs3dScanning";
            case ErrorType::NoScanningDeviceDetected: return "NoScanningDeviceDetected";
            case ErrorType::Iscan3dEncoderIndexSearchNotCompleted: return "Iscan3dEncoderIndexSearchNotCompleted";
            case ErrorType::StylusNotCalibrated: return "StylusNotCalibrated";
            case ErrorType::HomingNotPerformed: return "HomingNotPerformed";
            case ErrorType::FirmwareTooOld: return "FirmwareTooOld";
            default: return "UnknownError";
            }
        }

        std::string FormatApiErrorMessage(const std::string& action, ErrorType code)
        {
            return action + " 失败: 错误码 " + std::to_string(static_cast<unsigned int>(code)) +
                " (" + DescribeApiError(code) + ")";
        }
    }

    TrackerDevice::TrackerDevice()
    {
    }

    void TrackerDevice::TaskCallback(const api::lt::TaskResult& taskStatus, void* context)
    {
        auto* pThis = static_cast<TrackerDevice*>(context);
        if (!pThis)
        {
            OperationLogger::Instance().Log(LogLevel::Error, u8"任务回调上下文为空。\n");
            return;
        }

        std::ostringstream callbackLog;
        callbackLog << "任务回调返回: task=" << static_cast<unsigned int>(taskStatus.task)
            << ", success=" << (taskStatus.success ? "true" : "false")
            << ", errorCode=" << static_cast<unsigned int>(taskStatus.error_code)
            << " (" << DescribeApiError(taskStatus.error_code) << ")";
        OperationLogger::Instance().Log(LogLevel::Info, callbackLog.str());

        if (taskStatus.task == TaskMode::Connect)
        {
            pThis->connectSuccess_ = taskStatus.success;
            pThis->realTimeConnected_ = taskStatus.success;
            pThis->isConnecting_ = false;
            pThis->connectCompleted_ = true;

            if (taskStatus.success)
            {
                OperationLogger::Instance().Log(LogLevel::Info, u8"跟踪仪连接成功，设备已准备就绪。\n");
            }
            else
            {
                OperationLogger::Instance().Log(LogLevel::Error,
                    FormatApiErrorMessage("跟踪仪连接任务", taskStatus.error_code));
            }
        }
        else if (taskStatus.task == TaskMode::SpiralSearch)
        {
            pThis->isSearchingTarget_ = false;

            if (taskStatus.success)
            {
                OperationLogger::Instance().Log(LogLevel::Info, u8"螺旋搜索成功，已成功锁定靶标球！");
            }
            else
            {
                OperationLogger::Instance().Log(LogLevel::Warning,
                    FormatApiErrorMessage("螺旋搜索任务", taskStatus.error_code));
            }
        }
    }

    void TrackerDevice::StartAutoDetect()
    {
        if (isSearching_)
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"自动搜索请求被忽略，当前已有搜索任务在运行。");
            return;
        }

        isSearching_ = true;
        detectCompleted_ = false;
        detectSuccess_ = false;
        detectedIp_.clear();

        OperationLogger::Instance().Log(LogLevel::Warning, u8"当前 SDK 未提供自动搜索接口，请手动输入设备 IP 后再连接。");

        detectCompleted_ = true;
        isSearching_ = false;
    }

    void TrackerDevice::StartConnectAsync(const std::string& ipAddress)
    {
        if (isConnecting_ || IsConnected())
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"连接请求被忽略，当前正在连接或已经连接。");
            return;
        }

        isConnecting_ = true;
        connectCompleted_ = false;
        connectSuccess_ = false;

        OperationLogger::Instance().Log(LogLevel::Info, u8"正在后台尝试连接跟踪仪: " + ipAddress);

        const std::string dllDirectory = WideToUtf8(GetExecutableDirectory());
        const ErrorType res = apiDevice_.Connect(api::lt::DeviceModel::RadianPro, ipAddress.c_str(), nullptr,
            &TrackerDevice::TaskCallback, this, nullptr, dllDirectory.c_str());
        if (res != ErrorType::Success)
        {
            connectSuccess_ = false;
            realTimeConnected_ = false;
            isConnecting_ = false;
            connectCompleted_ = true;
            OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("跟踪仪连接", res));
            return;
        }

        OperationLogger::Instance().Log(LogLevel::Info, u8"SDK Connect() 已成功提交，等待任务回调。\n");
    }

    void TrackerDevice::StartDisconnectAsync()
    {
        if (isDisconnecting_ || !IsConnected())
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"断开请求被忽略，当前并未处于有效连接状态或已有断开任务在运行。");
            return;
        }

        isDisconnecting_ = true;
        disconnectCompleted_ = false;
        realTimeConnected_ = false;

        OperationLogger::Instance().Log(LogLevel::Info, u8"正在后台安全断开跟踪仪...");

        std::thread([this]() {
            const ErrorType abortRes = apiDevice_.AbortProcedure();
            if (abortRes != ErrorType::Success && abortRes != ErrorType::DeviceNotConnected)
            {
                OperationLogger::Instance().Log(LogLevel::Warning, FormatApiErrorMessage("AbortProcedure", abortRes));
            }

            const ErrorType disconnectRes = apiDevice_.Disconnect();
            if (disconnectRes != ErrorType::Success)
            {
                OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("Disconnect", disconnectRes));
            }
            else
            {
                OperationLogger::Instance().Log(LogLevel::Info, u8"跟踪仪已断开连接。\n");
            }

            connectSuccess_ = false;
            isConnecting_ = false;
            connectCompleted_ = false;
            isSearchingTarget_ = false;

            isDisconnecting_ = false;
            disconnectCompleted_ = true;
            }).detach();
    }

    void TrackerDevice::StartMeasureAsync()
    {
        if (isMeasuring_ || !IsConnected())
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"测量请求被忽略，当前正在测量或设备未连接。");
            return;
        }

        isMeasuring_ = true;
        measureCompleted_ = false;
        measureSuccess_ = false;

        OperationLogger::Instance().Log(LogLevel::Info, u8"正在后台启动单点测量...");

        std::thread([this]() {
            const ErrorType trackRes = SwitchToTrack();
            if (trackRes != ErrorType::Success)
            {
                OperationLogger::Instance().Log(LogLevel::Warning, FormatApiErrorMessage("SwitchToTrack", trackRes));
            }

            api::lt::Vector3<float> measuredPoint{};
            float rmsError = 0.0f;
            constexpr uint32_t averagingTimeMs = 1000;

            const ErrorType measureRes = apiDevice_.GetSinglePointMeasurement(averagingTimeMs, measuredPoint, rmsError);
            if (measureRes == ErrorType::Success)
            {
                lastMeasurePoint_.x = measuredPoint.x;
                lastMeasurePoint_.y = measuredPoint.y;
                lastMeasurePoint_.z = measuredPoint.z;
                lastMeasureAvg_ = static_cast<double>(averagingTimeMs);
                lastMeasureMax_ = 0.0;
                lastMeasureRms_ = rmsError;

                measureSuccess_ = true;
                std::ostringstream measureLog;
                measureLog << "单点测量成功: x=" << lastMeasurePoint_.x
                    << ", y=" << lastMeasurePoint_.y
                    << ", z=" << lastMeasurePoint_.z
                    << ", avg=" << lastMeasureAvg_
                    << ", rms=" << lastMeasureRms_;
                OperationLogger::Instance().Log(LogLevel::Info, measureLog.str());
            }
            else
            {
                measureSuccess_ = false;
                OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("GetSinglePointMeasurement", measureRes));
            }

            isMeasuring_ = false;
            measureCompleted_ = true;
            }).detach();
    }

    void TrackerDevice::StartSearchTargetAsync()
    {
        if (isSearchingTarget_ || !IsConnected())
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"靶标搜索请求被忽略，当前正在搜索或设备未连接。");
            return;
        }

        isSearchingTarget_ = true;

        OperationLogger::Instance().Log(LogLevel::Info, u8"准备自动搜索靶标球...");

        std::thread([this]() {
            const ErrorType trackRes = SwitchToTrack();
            if (trackRes != ErrorType::Success)
            {
                OperationLogger::Instance().Log(LogLevel::Warning, FormatApiErrorMessage("SwitchToTrack", trackRes));
            }

            const ErrorType searchRes = apiDevice_.SpiralSearch(0.0f, 0.0f, 30000);
            if (searchRes != ErrorType::Success)
            {
                isSearchingTarget_ = false;
                OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("SpiralSearch", searchRes));
                return;
            }

            OperationLogger::Instance().Log(LogLevel::Info, u8"螺旋搜索任务已提交，等待任务回调。\n");
            }).detach();
    }

    bool TrackerDevice::IsConnected() const
    {
        return apiDevice_.IsDeviceConnected();
    }

    ErrorType TrackerDevice::SwitchToIdle()
    {
        if (!IsConnected())
        {
            return ErrorType::DeviceNotConnected;
        }

        const ErrorType res = apiDevice_.SwitchToIdle();
        if (res != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("SwitchToIdle", res));
        }
        return res;
    }

    ErrorType TrackerDevice::SwitchToPosition()
    {
        if (!IsConnected())
        {
            return ErrorType::DeviceNotConnected;
        }

        const ErrorType res = apiDevice_.SwitchToPosition();
        if (res != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("SwitchToPosition", res));
        }
        return res;
    }

    ErrorType TrackerDevice::SwitchToTrack()
    {
        if (!IsConnected())
        {
            return ErrorType::DeviceNotConnected;
        }

        const ErrorType res = apiDevice_.SwitchToTrack();
        if (res != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("SwitchToTrack", res));
        }
        return res;
    }

    ErrorType TrackerDevice::EnableCameraSearch(bool enable)
    {
        if (!IsConnected())
        {
            return ErrorType::DeviceNotConnected;
        }

        const ErrorType res = apiDevice_.EnableCameraSearch(enable);
        if (res != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Error,
                FormatApiErrorMessage(enable ? "EnableCameraSearch(true)" : "EnableCameraSearch(false)", res));
        }
        return res;
    }

    ErrorType TrackerDevice::Home(api::lt::SmrSize smr_size)
    {
        if (!IsConnected())
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"Home请求被忽略，设备未连接。");
            return ErrorType::DeviceNotConnected;
        }

        const ErrorType res = apiDevice_.Home(smr_size);
        if (res != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("Home", res));
        }
        else
        {
            OperationLogger::Instance().Log(LogLevel::Info, u8"Home 任务已成功提交，等待回调。");
        }
        return res;
    }

    namespace
    {
        constexpr float kAzimuthPositiveTarget = 320.0f;
        constexpr float kAzimuthNegativeTarget = -320.0f;
        constexpr float kElevationPositiveTarget = 243.0f;
        constexpr float kElevationNegativeTarget = -63.0f;
    }

    void TrackerDevice::MoveAzimuth(double stepDegrees)
    {
        if (!IsConnected())
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"方位角点动请求被忽略，设备未连接。");
            return;
        }

        const TrackerRealTimeStatus currentStatus = GetRealTimeStatus();
        const float targetAzimuth = stepDegrees >= 0.0 ? kAzimuthPositiveTarget : kAzimuthNegativeTarget;

        std::ostringstream oss;
        oss << "发送方位角 JogTo 指令, 当前俯仰角: " << currentStatus.elevation
            << ", 目标方位角: " << targetAzimuth;
        OperationLogger::Instance().Log(LogLevel::Info, oss.str());

        const ErrorType positionRes = SwitchToPosition();
        if (positionRes != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Warning, FormatApiErrorMessage("SwitchToPosition", positionRes));
        }

        const ErrorType res = apiDevice_.JogTo(targetAzimuth, static_cast<float>(currentStatus.elevation),false);
        if (res != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("方位角 JogTo", res));
        }
    }

    void TrackerDevice::MoveElevation(double stepDegrees)
    {
        if (!IsConnected())
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"俯仰角点动请求被忽略，设备未连接。");
            return;
        }

        const TrackerRealTimeStatus currentStatus = GetRealTimeStatus();
        const float targetElevation = stepDegrees >= 0.0 ? kElevationPositiveTarget : kElevationNegativeTarget;

        std::ostringstream oss;
        oss << "发送俯仰角 JogTo 指令, 当前方位角: " << currentStatus.azimuth
            << ", 目标俯仰角: " << targetElevation;
        OperationLogger::Instance().Log(LogLevel::Info, oss.str());

        const ErrorType positionRes = SwitchToPosition();
        if (positionRes != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Warning, FormatApiErrorMessage("SwitchToPosition", positionRes));
        }

        const ErrorType res = apiDevice_.JogTo(static_cast<float>(currentStatus.azimuth), targetElevation,false);
        if (res != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("俯仰角 JogTo", res));
        }
    }

    ErrorType TrackerDevice::StopMotionByModeChange()
    {
        if (!IsConnected())
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"模式切换停运动请求被忽略，设备未连接。");
            return ErrorType::DeviceNotConnected;
        }

        OperationLogger::Instance().Log(LogLevel::Info, u8"通过切换模式停止运动: 先切换到 Idle，再切换回 Position。");

        ErrorType firstRes = SwitchToIdle();
        if (firstRes != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Warning, FormatApiErrorMessage("SwitchToIdle", firstRes));
        }

        const ErrorType secondRes = SwitchToPosition();
        if (secondRes != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("SwitchToPosition", secondRes));
            return secondRes;
        }

        return firstRes;
    }

    void TrackerDevice::StopMotion()
    {
        if (!IsConnected())
        {
            OperationLogger::Instance().Log(LogLevel::Warning, u8"急停请求被忽略，设备未连接。");
            return;
        }

        OperationLogger::Instance().Log(LogLevel::Info, u8"发送打断运动(急停)指令...");
        const ErrorType res = apiDevice_.AbortProcedure();
        if (res != ErrorType::Success)
        {
            OperationLogger::Instance().Log(LogLevel::Error, FormatApiErrorMessage("AbortProcedure", res));
        }
    }

    TrackerRealTimeStatus TrackerDevice::GetRealTimeStatus() const
    {
        TrackerRealTimeStatus status{};

        if (!IsConnected())
        {
            return status;
        }

        api::lt::RealTimeInfo rtInfo{};
        if (apiDevice_.GetRealTimeData(rtInfo) != ErrorType::Success)
        {
            return status;
        }

        status.azimuth = rtInfo.az;
        status.elevation = rtInfo.el;
        status.distance = rtInfo.distance;
        status.x = rtInfo.x;
        status.y = rtInfo.y;
        status.z = rtInfo.z;
        status.intensity = rtInfo.diagnostic_info.laser_intensity;
        status.isLaserPathError = !rtInfo.is_measurement_valid;
        status.isWarmingUp = !rtInfo.is_warmed_up;
        status.temperature = rtInfo.body_temperature;
        status.pressure = 0.0;
        status.humidity = 0.0;
        status.operationMode = rtInfo.operation_mode;

        return status;
    }

} // namespace senpec
