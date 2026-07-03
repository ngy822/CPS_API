#pragma once

#include <array>
#include <chrono>
#include <string>
#include <vector>

namespace senpec
{
    struct Pose
    {
        double x;
        double y;
        double z;
        double rx;
        double ry;
        double rz;
    };

    struct KinematicsResult
    {
        Pose currentPose;
        Pose targetPose;
        Pose relativePose;
        std::array<double, 6> mechanismDelta;
    };

    struct MeasurementRecord
    {
        int index;
        double x;
        double y;
        double z;
        std::string targetSize;
        std::chrono::system_clock::time_point timestamp;
        KinematicsResult kinematics;
    };

    class MeasurementStore
    {
    public:
        void AddRecord(const MeasurementRecord& record);
        void Clear();
        const std::vector<MeasurementRecord>& GetRecords() const;
        bool ExportCsv(const std::string& path) const;

    private:
        std::vector<MeasurementRecord> records_;
    };
}
