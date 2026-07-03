#include "core/MeasurementStore.h"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "core/OperationLogger.h"

namespace senpec
{
    void MeasurementStore::AddRecord(const MeasurementRecord& record)
    {
        records_.push_back(record);
        std::ostringstream stream;
        stream << "测量数据存储: index=" << record.index
            << ", x=" << record.x
            << ", y=" << record.y
            << ", z=" << record.z
            << ", target=" << record.targetSize;
        OperationLogger::Instance().Log(LogLevel::Info, stream.str());
    }

    void MeasurementStore::Clear()
    {
        records_.clear();
        OperationLogger::Instance().Log(LogLevel::Info, "测量数据已清空。");
    }

    const std::vector<MeasurementRecord>& MeasurementStore::GetRecords() const
    {
        return records_;
    }

    static std::string FormatTime(const std::chrono::system_clock::time_point& timePoint)
    {
        const std::time_t timeValue = std::chrono::system_clock::to_time_t(timePoint);
        std::tm tmValue{};
#ifdef _WIN32
        localtime_s(&tmValue, &timeValue);
#else
        localtime_r(&timeValue, &tmValue);
#endif
        std::ostringstream stream;
        stream << std::put_time(&tmValue, "%Y-%m-%d %H:%M:%S");
        return stream.str();
    }

    bool MeasurementStore::ExportCsv(const std::string& path) const
    {
        // 1. 【核心修复】使用 u8path() 将 ImGui 传来的 UTF-8 字符串安全地解析为内部路径
        std::filesystem::path fullPath = std::filesystem::absolute(std::filesystem::u8path(path));

        // 2. ofstream 接受 path 对象时，底层直接调 Windows 的 _wfopen 宽字符 API，完美支持中文路径！
        std::ofstream file(fullPath, std::ios::trunc);
        if (!file.is_open())
        {
            // 3. 【核心修复】使用 u8string() 获取安全的 UTF-8 字符串输出给日志，彻底告别乱码
            OperationLogger::Instance().Log(LogLevel::Error, "导出CSV失败: 无法打开文件 " + fullPath.u8string());
            return false;
        }

        // 精简表头：只保留界面上展示的 6 列
        file << "Index,X,Y,Z,TargetSize,Timestamp\n";

        for (const auto& record : records_)
        {
            // 对应填入 6 列数据
            file << record.index << ","
                << record.x << ","
                << record.y << ","
                << record.z << ","
                << record.targetSize << ","
                << FormatTime(record.timestamp) << "\n";
        }

        // 成功时同样使用 u8string() 写入日志
        OperationLogger::Instance().Log(LogLevel::Info, "导出CSV成功: " + fullPath.u8string());
        return true;
    }
}