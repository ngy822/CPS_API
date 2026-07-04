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
            const DigitalTwinPublisher::PoseVector& currentPose,
            const DigitalTwinPublisher::PoseVector& targetPose)
        {
            const auto isFinite = [](double value) {
                return std::isfinite(value);
                };

            if (!std::all_of(currentPose.begin(), currentPose.end(), isFinite) ||
                !std::all_of(targetPose.begin(), targetPose.end(), isFinite))
            {
                OperationLogger::Instance().Log(
                    LogLevel::Error,
                    u8"CPS 位姿发布被拒绝：当前位姿或目标位姿包含 NaN/Inf。");
                return false;
            }

            {
                std::lock_guard<std::mutex> lock(poseMutex_);
                currentPose_ = currentPose;
                targetPose_ = targetPose;
                hasPose_ = true;
                ++poseVersion_;
            }

            wakeup_.notify_all();

            std::ostringstream stream;
            stream << "CPS 位姿已更新: current=["
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

            while (running_.load())
            {
                DigitalTwinPublisher::PoseVector currentPose{};
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
                        currentPose = currentPose_;
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
                std::copy(currentPose.begin(), currentPose.end(), currentMessage.pose);
                std::copy(targetPose.begin(), targetPose.end(), targetMessage.pose);

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

                if (currentResult == 0 && targetResult == 0)
                {
                    if (lastDeliveredVersion != version)
                    {
                        OperationLogger::Instance().Log(
                            LogLevel::Info,
                            u8"CPS 当前位姿(0x509)与目标位姿(0x50A)已开始按 50 ms 周期发布。");
                        lastDeliveredVersion = version;
                    }

                    if (previousCurrentError != 0 || previousTargetError != 0)
                    {
                        OperationLogger::Instance().Log(
                            LogLevel::Info,
                            u8"CPS 位姿发送已恢复正常。");
                    }
                }
                else if (currentResult != previousCurrentError || targetResult != previousTargetError)
                {
                    OperationLogger::Instance().Log(
                        LogLevel::Error,
                        u8"CPS 位姿发送失败：当前位姿错误码=" + std::to_string(currentResult) +
                        u8"，目标位姿错误码=" + std::to_string(targetResult));
                }

                previousCurrentError = currentResult;
                previousTargetError = targetResult;
            }
        }

        CCPSAPI* api_{ nullptr };
        std::unique_ptr<CpsEventHandler> handler_;

        std::atomic<bool> connected_{ false };
        std::atomic<bool> subscribed_{ false };
        std::atomic<bool> running_{ false };

        mutable std::mutex poseMutex_;
        std::condition_variable wakeup_;
        DigitalTwinPublisher::PoseVector currentPose_{};
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
        const PoseVector& currentPose,
        const PoseVector& targetPose)
    {
        return impl_ && impl_->PublishPosePair(currentPose, targetPose);
    }

    bool DigitalTwinPublisher::IsConnected() const noexcept
    {
        return impl_ && impl_->IsConnected();
    }
}
