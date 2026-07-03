#include "core/OperationLogger.h"

#define SPDLOG_HEADER_ONLY
#define FMT_HEADER_ONLY

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <windows.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <shellapi.h>

namespace senpec
{
    spdlog::level::level_enum OperationLogger::ToSpdLevel(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Debug:
            return spdlog::level::debug;
        case LogLevel::Info:
            return spdlog::level::info;
        case LogLevel::Warning:
            return spdlog::level::warn;
        case LogLevel::Error:
            return spdlog::level::err;
        default:
            return spdlog::level::info;
        }
    }

    static std::string FormatFileStamp()
    {
        const std::time_t nowTime = std::time(nullptr);
        std::tm tmValue{};
#ifdef _WIN32
        localtime_s(&tmValue, &nowTime);
#else
        localtime_r(&nowTime, &tmValue);
#endif

        std::ostringstream stream;
        stream << std::put_time(&tmValue, "%Y%m%d_%H%M%S");
        return stream.str();
    }

    OperationLogger& OperationLogger::Instance()
    {
        static OperationLogger instance;
        return instance;
    }

    OperationLogger::OperationLogger()
    {
        const std::filesystem::path logDir = std::filesystem::current_path() / "logs";
        std::error_code ec;
        std::filesystem::create_directories(logDir, ec);

        // 使用 std::filesystem::path 来构建完整路径，它在内部处理了宽字符问题
        std::filesystem::path fullPath = logDir / ("senpec_" + FormatFileStamp() + ".log");
        logFilePath_ = fullPath.string();

        // 现代 C++ 建议直接传入 path 对象给 ifstream，完美避开中文路径打开失败的问题
        std::ofstream file(fullPath, std::ios::binary | std::ios::app);
        if (file.is_open() && file.tellp() == 0)
        {
            // 写入 UTF-8 BOM 头
            const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
            file.write(reinterpret_cast<const char*>(bom), sizeof(bom));
            file.flush();
        }

        // spdlog 同样直接支持传入完整路径字符串
        logger_ = spdlog::basic_logger_mt("senpec_file_logger", logFilePath_, false);
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");
        logger_->flush_on(spdlog::level::trace);

        Log(LogLevel::Info, "日志系统已启动。");
    }

    void OperationLogger::Log(LogLevel level, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!logger_)
        {
            return;
        }

        logger_->log(ToSpdLevel(level), "{}", message);
        logger_->flush();
    }

    void OperationLogger::Log(const std::string& message)
    {
        Log(LogLevel::Info, message);
    }

    const std::string& OperationLogger::GetLogFilePath() const
    {
        return logFilePath_;
    }

    bool OperationLogger::OpenLogFile() const
    {
        // 【关键修复】使用 path::c_str() 配合 ShellExecuteW，彻底解决中文路径无法打开日志文件的 Bug
        std::filesystem::path p(logFilePath_);
        const HINSTANCE result = ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return reinterpret_cast<INT_PTR>(result) > 32;
    }
}