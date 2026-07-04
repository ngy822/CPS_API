#pragma once

#include <array>
#include <memory>

namespace senpec
{
    class DigitalTwinPublisher
    {
    public:
        using PoseMatrix = std::array<double, 16>;
        using PoseVector = std::array<double, 6>;

        DigitalTwinPublisher();
        ~DigitalTwinPublisher();

        DigitalTwinPublisher(const DigitalTwinPublisher&) = delete;
        DigitalTwinPublisher& operator=(const DigitalTwinPublisher&) = delete;

        // 缓存并持续发布最新的当前/目标位姿。
        // 矩阵按 4x4 行主序排列，向量按 [X, Y, Z, Rx, Ry, Rz] 排列。
        bool PublishPosePair(
            const PoseMatrix& currentMatrix,
            const PoseVector& currentPose,
            const PoseMatrix& targetMatrix,
            const PoseVector& targetPose);

        bool IsConnected() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
