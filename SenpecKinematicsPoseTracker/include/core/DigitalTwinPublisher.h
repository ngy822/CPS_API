#pragma once

#include <array>
#include <memory>

namespace senpec
{
    class DigitalTwinPublisher
    {
    public:
        using PoseVector = std::array<double, 6>;

        DigitalTwinPublisher();
        ~DigitalTwinPublisher();

        DigitalTwinPublisher(const DigitalTwinPublisher&) = delete;
        DigitalTwinPublisher& operator=(const DigitalTwinPublisher&) = delete;

        // 缓存并持续发布最新的当前/目标 6-DOF 位姿。
        // 两组向量均按 [X, Y, Z, Rx, Ry, Rz] 排列。
        bool PublishPosePair(const PoseVector& currentPose, const PoseVector& targetPose);

        bool IsConnected() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
