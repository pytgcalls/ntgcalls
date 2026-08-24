//
// Created by Lauren on 26/03/24.
//

#pragma once
#include <api/ref_count.h>
#include <rtc_base/logging.h>
#include <wrtc/utils/safe_thread.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace ntgcalls::utils {

    class LogSink: public webrtc::LogSink, public webrtc::RefCountInterface {
    public:
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

        static void register_logger(std::function<void(LogMessage)> callback);

        explicit LogSink();

        ~LogSink() override;

        void OnLogMessage(const std::string& msg, webrtc::LoggingSeverity severity, const char* tag) override;

        void OnLogMessage(const std::string& message, webrtc::LoggingSeverity severity) override;

        void OnLogMessage(const std::string& message) override;

        static void get_or_create();

        static void un_ref();

    private:
        static Level parse_severity(webrtc::LoggingSeverity severity);

        static uint32_t parse_line_number(const std::string& message);

        void register_log_message(const std::string& message, webrtc::LoggingSeverity severity) const;

        static webrtc::scoped_refptr<LogSink> instance_;
        static std::mutex mutex_;
        static uint32_t references_;
        static wrtc::utils::synchronized_callback<void(LogMessage)> on_log_message_;
        std::unique_ptr<wrtc::utils::SafeThread> thread_;
    };

} // ntgcalls::utils
