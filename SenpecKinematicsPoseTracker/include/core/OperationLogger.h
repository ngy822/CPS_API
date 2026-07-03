#pragma once

#include <memory>
#include <mutex>
#include <string>

#include <spdlog/spdlog.h>

namespace senpec
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error,
    };

    class OperationLogger
    {
    public:
        static OperationLogger& Instance();

        void Log(LogLevel level, const std::string& message);
        void Log(const std::string& message);
        const std::string& GetLogFilePath() const;
        bool OpenLogFile() const;

    private:
        OperationLogger();

        static spdlog::level::level_enum ToSpdLevel(LogLevel level);

        std::string logFilePath_;
        std::shared_ptr<spdlog::logger> logger_;
        mutable std::mutex mutex_;
    };
}