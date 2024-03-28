//
// Created by Laky64 on 26/03/2024.
//

#include "log_sink_impl.hpp"

#include <chrono>

namespace ntgcalls {
    std::string LogSinkImpl::severityToString(const rtc::LoggingSeverity severity) {
        switch (severity) {
            case rtc::LS_VERBOSE:
                return "VERBOSE";
            case rtc::LS_INFO:
                return "INFO";
            case rtc::LS_WARNING:
                return "WARNING";
            case rtc::LS_ERROR:
                return "ERROR";
            case rtc::LS_NONE:
                return "NONE";
            default:
                return "UNKNOWN";
        }
    }

    LogSinkImpl::LogSinkImpl(const std::string& logPath, const bool allowWebrtcLogs): allowWebrtcLogs(allowWebrtcLogs) {
        _file.open(logPath);
    }

    void LogSinkImpl::OnLogMessage(const std::string& msg, const rtc::LoggingSeverity severity, const char* tag) {
        OnLogMessage(std::string(tag) + ": " + msg, severity);
    }

    void LogSinkImpl::OnLogMessage(const std::string& message, const rtc::LoggingSeverity severity) {
        OnLogMessage("[" + severityToString(severity) + "] " + message);
    }

    void LogSinkImpl::OnLogMessage(const std::string& message) {
        if (!allowWebrtcLogs && message.find(".cc:") != std::string::npos) {
            return;
        }
        std::ostringstream logStream;
        const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
        const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
        std::tm timeinfo{};
#ifdef _WIN32
        localtime_s(&timeinfo, &timeNow);
#else
        localtime_r(&timeNow, &timeinfo);
#endif
        logStream << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S:") << std::setw(3) << std::setfill('0') << nowMs << " " << message;
        auto& outputStream = _file.is_open() ? static_cast<std::ostream&>(_file) : _data;
        outputStream << logStream.str();
    }

    std::string LogSinkImpl::result() const {
        return _data.str();
    }
} // ntgcalls