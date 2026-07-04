#include "core/DigitalTwinPublisher.h"

#include "core/OperationLogger.h"

#include "CPSAPI/CPSAPI.h"
#include "CPSAPI/CPSDef.h"

#pragma execution_character_set("utf-8")

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace senpec
{
    namespace
    {
        constexpr const char* kBusServerIp = "127.0.0.1";
        constexpr int kBusServerPort = 5000;
        constexpr const char* kLogServerIp = "127.0.0.1";
        constexpr int kLogServerPort = 8888;
        constexpr std::chrono::milliseconds kPublishInterval{ 50 };

        static_assert(sizeof(ST_POSE) == sizeof(double) * 6, "Unexpected ST_POSE layout");
        static_assert(
            sizeof(ST_MATRIX4X4) == sizeof(double) * 16,
            "Unexpected ST_MATRIX4X4 layout");

        std::string ReadFixedString(const char* text, std::size_t capacity)
        {
            if (!text || capacity == 0)
            {
                return {};
            }

            const char* end = std::find(text, text + capacity, '\0');
            return std::string(text, end);
        }

        class CpsEventHandler final : public CCPSEventHandler
        {
        public:
            CpsEventHandler(
                CCPSAPI* api,
                std::atomic<bool>& connected,
                std::atomic<bool>& subscribed)
                : api_(api)
                , connected_(connected)
                , subscribed_(subscribed)
            {
            }

            void OnConnected() override
            {
                connected_.store(true);

                if (!api_)
                {
                    subscribed_.store(false);
                    OperationLogger::Instance().Log(
                        LogLevel::Error,
                        u8"CPS 总线已连接，但 API 实例为空，无法订阅数字孪生设备。");
                    return;
                }

                const int result = api_->SubscribeDevice(CPS_DT_DEVICE_ID);
                subscribed_.store(result == 0);

                if (result == 0)
                {
                    OperationLogger::Instance().Log(
                        LogLevel::Info,
                        u8"CPS 总线连接成功，已订阅数字孪生设备 401。");
                }
                else
                {
                    OperationLogger::Instance().Log(
                        LogLevel::Error,
                        u8"CPS 总线已连接，但订阅设备 401 失败，错误码=" + std::to_string(result));
                }
            }

            void OnDisconnected() override
            {
                subscribed_.store(false);
                connected_.store(false);
                OperationLogger::Instance().Log(
                    LogLevel::Warning,
                    u8"CPS 总线连接已断开，位姿数据将保留并等待连接恢复。");
            }

            void OnMsg(
                std::uint32_t fromId,
                std::uint32_t messageType,
                const char* data,
                std::uint32_t messageLength) override
            {
                if (!data || messageLength == 0)
                {
                    OperationLogger::Instance().Log(
                        LogLevel::Warning,
                        u8"CPS 总线收到空消息，来源=" + std::to_string(fromId));
                    return;
                }

                std::ostringstream stream;
                if (messageType == MSG_CMD_INIT_RSP && messageLength >= sizeof(ST_CMDInitRsp))
                {
                    const auto* response = reinterpret_cast<const ST_CMDInitRsp*>(data);
                    stream << "CPS 初始化响应: source=" << fromId
                        << ", request=" << response->req_no
                        << ", code=" << response->rsp.error_code
                        << ", message="
                        << ReadFixedString(response->rsp.error_msg, sizeof(response->rsp.error_msg));
                    OperationLogger::Instance().Log(LogLevel::Info, stream.str());
                    return;
                }

                if (messageType == MSG_CMD_STOP_RSP && messageLength >= sizeof(ST_CMDStopRsp))
                {
                    const auto* response = reinterpret_cast<const ST_CMDStopRsp*>(data);
                    stream << "CPS 停止响应: source=" << fromId
                        << ", request=" << response->req_no
                        << ", code=" << response->rsp.error_code;
                    OperationLogger::Instance().Log(LogLevel::Info, stream.str());
                    return;
                }

                if (messageType == MSG_RSP_INFO && messageLength >= sizeof(ST_Rsp))
                {
                    const auto* response = reinterpret_cast<const ST_Rsp*>(data);
                    stream << "CPS 通用响应: source=" << fromId
                        << ", request=" << response->req_no
                        << ", code=" << response->rsp.error_code
                        << ", message="
                        << ReadFixedString(response->rsp.error_msg, sizeof(response->rsp.error_msg));
                    OperationLogger::Instance().Log(LogLevel::Info, stream.str());
                    return;
                }

                stream << "CPS 收到未处理消息: source=" << fromId
                    << ", type=0x" << std::hex << messageType << std::dec
                    << ", length=" << messageLength;
                OperationLogger::Instance().Log(LogLevel::Debug, stream.str());
            }

        private:
            CCPSAPI* api_;
            std::atomic<bool>& connected_;
            std::atomic<bool>& subscribed_;
        };
    }

    class DigitalTwinPublisher::Impl
    {
    public:
        Impl()
        {
            api_ = CCPSAPI::CreateAPI();
            if (!api_)
            {
                OperationLogger::Instance().Log(
                    LogLevel::Error,
                    u8"创建 CPS API 实例失败，数字孪生位姿发布不可用。");
                return;
            }

            handler_ = std::make_unique<CpsEventHandler>(api_, connected_, subscribed_);

            const int result = api_->Init(
                E_CPS_TYPE_APP,
                0,
                CONTROL_SERVER_APP_ID,
                kBusServerIp,
                kBusServerPort,
                kLogServerIp,
                kLogServerPort,
                handler_.get());

            if (result != 0)
            {
                OperationLogger::Instance().Log(
                    LogLevel::Error,
                    u8"CPS API 初始化失败，错误码=" + std::to_string(result));
                api_->Release();
                api_ = nullptr;
                handler_.reset();
                return;
            }

            running_.store(true);
            senderThread_ = std::thread(&Impl::SenderLoop, this);

            OperationLogger::Instance().Log(
                LogLevel::Info,
                u8"CPS 位姿发布器已启动，正在连接 127.0.0.1:5000。");
        }

        ~Impl()
        {
            running_.store(false);
            wakeup_.notify_all();

            if (senderThread_.joinable())
            {
                senderThread_.join();
            }

            if (api_)
            {
                if (connected_.load() && subscribed_.load())
                {
                    const int result = api_->UnSubscribeDevice(CPS_DT_DEVICE_ID);
                    if (result != 0)
                    {
                        OperationLogger::Instance().Log(
                            LogLevel::Warning,
                            u8"取消订阅 CPS 设备 401 失败，错误码=" + std::to_string(result));
                    }
                }

                api_->Release();
                api_ = nullptr;
            }

            handler_.reset();
            subscribed_.store(false);
            connected_.store(false);

            OperationLogger::Instance().Log(LogLevel::Info, u8"CPS 位姿发布器已停止。");
        }

        bool PublishPosePair(
            const DigitalTwinPublisher::PoseMatrix& currentMatrix,
            const DigitalTwinPublisher::PoseVector& currentPose,
            const DigitalTwinPublisher::PoseMatrix& targetMatrix,
            const DigitalTwinPublisher::PoseVector& targetPose)
        {
            const auto isFinite = [](double value) {
                return std::isfinite(value);
                };

            if (!std::all_of(currentMatrix.begin(), currentMatrix.end(), isFinite) ||
                !std::all_of(currentPose.begin(), currentPose.end(), isFinite) ||
                !std::all_of(targetMatrix.begin(), targetMatrix.end(), isFinite) ||
                !std::all_of(targetPose.begin(), targetPose.end(), isFinite))
            {
                OperationLogger::Instance().Log(
                    LogLevel::Error,
                    u8"CPS 位姿发布被拒绝：当前/目标矩阵或 3平3转分量包含 NaN/Inf。");
                return false;
            }

            {
                std::lock_guard<std::mutex> lock(poseMutex_);
                currentMatrix_ = currentMatrix;
                currentPose_ = currentPose;
                targetMatrix_ = targetMatrix;
                targetPose_ = targetPose;
                hasPose_ = true;
                ++poseVersion_;
            }

            wakeup_.notify_all();

            std::ostringstream stream;
            stream << "CPS 位姿矩阵及分量已更新: current=["
                << currentPose[0] << ", " << currentPose[1] << ", " << currentPose[2] << ", "
                << currentPose[3] << ", " << currentPose[4] << ", " << currentPose[5] << "]"
                << ", target=["
                << targetPose[0] << ", " << targetPose[1] << ", " << targetPose[2] << ", "
                << targetPose[3] << ", " << targetPose[4] << ", " << targetPose[5] << "]";
            OperationLogger::Instance().Log(LogLevel::Info, stream.str());
            return true;
        }

        bool IsConnected() const noexcept
        {
            return connected_.load() && subscribed_.load();
        }

    private:
        void SenderLoop()
        {
            std::uint64_t lastDeliveredVersion = 0;
            int previousCurrentError = 0;
            int previousTargetError = 0;
            int previousCurrentMatrixError = 0;
            int previousTargetMatrixError = 0;

            while (running_.load())
            {
                DigitalTwinPublisher::PoseMatrix currentMatrix{};
                DigitalTwinPublisher::PoseVector currentPose{};
                DigitalTwinPublisher::PoseMatrix targetMatrix{};
                DigitalTwinPublisher::PoseVector targetPose{};
                std::uint64_t version = 0;
                bool hasPose = false;

                {
                    std::unique_lock<std::mutex> lock(poseMutex_);
                    wakeup_.wait_for(lock, kPublishInterval);
                    if (!running_.load())
                    {
                        break;
                    }

                    hasPose = hasPose_;
                    if (hasPose)
                    {
                        currentMatrix = currentMatrix_;
                        currentPose = currentPose_;
                        targetMatrix = targetMatrix_;
                        targetPose = targetPose_;
                        version = poseVersion_;
                    }
                }

                if (!hasPose || !api_ || !connected_.load() || !subscribed_.load())
                {
                    continue;
                }

                ST_POSE currentMessage{};
                ST_POSE targetMessage{};
                ST_MATRIX4X4 currentMatrixMessage{};
                ST_MATRIX4X4 targetMatrixMessage{};
                std::copy(currentPose.begin(), currentPose.end(), currentMessage.pose);
                std::copy(targetPose.begin(), targetPose.end(), targetMessage.pose);
                std::copy(
                    currentMatrix.begin(),
                    currentMatrix.end(),
                    currentMatrixMessage.matrix);
                std::copy(
                    targetMatrix.begin(),
                    targetMatrix.end(),
                    targetMatrixMessage.matrix);

                const int currentResult = api_->SendAPPMsg(
                    CPS_DT_DEVICE_ID,
                    MSG_POSE,
                    reinterpret_cast<const char*>(&currentMessage),
                    sizeof(currentMessage));

                const int targetResult = api_->SendAPPMsg(
                    CPS_DT_DEVICE_ID,
                    MSG_TARGET_POSE,
                    reinterpret_cast<const char*>(&targetMessage),
                    sizeof(targetMessage));

                const int currentMatrixResult = api_->SendAPPMsg(
                    CPS_DT_DEVICE_ID,
                    MSG_CURRENT_POSE_MATRIX,
                    reinterpret_cast<const char*>(&currentMatrixMessage),
                    sizeof(currentMatrixMessage));

                const int targetMatrixResult = api_->SendAPPMsg(
                    CPS_DT_DEVICE_ID,
                    MSG_TARGET_POSE_MATRIX,
                    reinterpret_cast<const char*>(&targetMatrixMessage),
                    sizeof(targetMatrixMessage));

                if (currentResult == 0 &&
                    targetResult == 0 &&
                    currentMatrixResult == 0 &&
                    targetMatrixResult == 0)
                {
                    if (lastDeliveredVersion != version)
                    {
                        OperationLogger::Instance().Log(
                            LogLevel::Info,
                            u8"CPS 当前分量(0x509)、目标分量(0x50A)、当前矩阵(0x50B)、目标矩阵(0x50C)已开始按 50 ms 周期发布。");
                        lastDeliveredVersion = version;
                    }

                    if (previousCurrentError != 0 ||
                        previousTargetError != 0 ||
                        previousCurrentMatrixError != 0 ||
                        previousTargetMatrixError != 0)
                    {
                        OperationLogger::Instance().Log(
                            LogLevel::Info,
                            u8"CPS 位姿发送已恢复正常。");
                    }
                }
                else if (currentResult != previousCurrentError ||
                    targetResult != previousTargetError ||
                    currentMatrixResult != previousCurrentMatrixError ||
                    targetMatrixResult != previousTargetMatrixError)
                {
                    OperationLogger::Instance().Log(
                        LogLevel::Error,
                        u8"CPS 位姿发送失败：当前分量=" + std::to_string(currentResult) +
                        u8"，目标分量=" + std::to_string(targetResult) +
                        u8"，当前矩阵=" + std::to_string(currentMatrixResult) +
                        u8"，目标矩阵=" + std::to_string(targetMatrixResult));
                }

                previousCurrentError = currentResult;
                previousTargetError = targetResult;
                previousCurrentMatrixError = currentMatrixResult;
                previousTargetMatrixError = targetMatrixResult;
            }
        }

        CCPSAPI* api_{ nullptr };
        std::unique_ptr<CpsEventHandler> handler_;

        std::atomic<bool> connected_{ false };
        std::atomic<bool> subscribed_{ false };
        std::atomic<bool> running_{ false };

        mutable std::mutex poseMutex_;
        std::condition_variable wakeup_;
        DigitalTwinPublisher::PoseMatrix currentMatrix_{};
        DigitalTwinPublisher::PoseVector currentPose_{};
        DigitalTwinPublisher::PoseMatrix targetMatrix_{};
        DigitalTwinPublisher::PoseVector targetPose_{};
        bool hasPose_{ false };
        std::uint64_t poseVersion_{ 0 };

        std::thread senderThread_;
    };

    DigitalTwinPublisher::DigitalTwinPublisher()
        : impl_(std::make_unique<Impl>())
    {
    }

    DigitalTwinPublisher::~DigitalTwinPublisher() = default;

    bool DigitalTwinPublisher::PublishPosePair(
        const PoseMatrix& currentMatrix,
        const PoseVector& currentPose,
        const PoseMatrix& targetMatrix,
        const PoseVector& targetPose)
    {
        return impl_ &&
            impl_->PublishPosePair(currentMatrix, currentPose, targetMatrix, targetPose);
    }

    bool DigitalTwinPublisher::IsConnected() const noexcept
    {
        return impl_ && impl_->IsConnected();
    }
}
