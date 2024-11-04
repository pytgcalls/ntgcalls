//
// Created by Laky64 on 26/03/2024.
//

#pragma once
#include <api/ref_count.h>
#include <rtc_base/thread.h>
#include <rtc_base/logging.h>
#include <ntgcalls/utils/binding_utils.hpp>
#include <wrtc/utils/syncronized_callback.hpp>

namespace ntgcalls {

    class LogSink: public rtc::LogSink, public webrtc::RefCountInterface {
    public:
#ifndef PYTHON_ENABLED
        enum class Level {
            Debug = 1 << 0,
            Info = 1 << 1,
            Warning = 1 << 2,
            Error = 1 << 3,
            Unknown = -1
        };

        enum class Source {
            WebRTC = 1 << 0,
            Self = 1 << 1
        };

        struct LogMessage {
            Level level;
            Source source;
            std::string file;
            uint32_t line;
            std::string message;
        };

        static void registerLogger(std::function<void(LogMessage)> callback);
#endif

        explicit LogSink();

        ~LogSink() override;

        void OnLogMessage(const std::string &msg, rtc::LoggingSeverity severity, const char *tag) override;

        void OnLogMessage(const std::string &message, rtc::LoggingSeverity severity) override;

        void OnLogMessage(const std::string &message) override;

        static void GetOrCreate();

        static void UnRef();

    private:
#ifdef PYTHON_ENABLED
        static py::object parseSeverity(rtc::LoggingSeverity severity);
#else
        static Level parseSeverity(rtc::LoggingSeverity severity);
#endif

        static uint32_t parseLineNumber(const std::string &message);

        void registerLogMessage(const std::string &message, rtc::LoggingSeverity severity) const;

        static rtc::scoped_refptr<LogSink> instance;
        static std::mutex mutex;
        static uint32_t references;
#ifdef PYTHON_ENABLED
        py::object rtcLogs;
        py::object ntgLogs;
#else
        static wrtc::synchronized_callback<LogMessage> onLogMessage;
#endif
        std::unique_ptr<rtc::Thread> thread;
    };

} // ntgcalls
