//
// Created by Lauren on 26/03/24.
//

#include <ntgcalls/utils/log_sink_impl.hpp>

#include <regex>
#include <sstream>
#include <rtc_base/ref_counted_object.h>

namespace ntgcalls::utils {
    webrtc::scoped_refptr<LogSink> LogSink::instance_ = nullptr;
    std::mutex LogSink::mutex_{};
    uint32_t LogSink::references_ = 0;
    const std::regex LogSink::message_pattern_(R"(\((.*)\.(.*):([0-9]+)\):\s?(.*))");
    wrtc::utils::synchronized_callback<void(LogSink::LogMessage)> LogSink::on_log_message_{};

    LogSink::LogSink() {
        thread_ = wrtc::utils::SafeThread::Create();
        thread_->SetName("LogSink", nullptr);
        thread_->Start();
#ifdef DEBUG
        webrtc::LogMessage::LogToDebug(webrtc::LS_VERBOSE);
#else
        webrtc::LogMessage::LogToDebug(webrtc::LS_INFO);
#endif
        webrtc::LogMessage::SetLogToStderr(false);
        webrtc::LogMessage::AddLogToStream(this, webrtc::LS_VERBOSE);
    }

    LogSink::~LogSink() {
        webrtc::LogMessage::RemoveLogToStream(this);
        thread_->Stop();
        thread_ = nullptr;
        on_log_message_ = nullptr;
    }

    LogSink::Level LogSink::parse_severity(const webrtc::LoggingSeverity severity) {
        switch (severity) {
        case webrtc::LS_VERBOSE:
            return Level::Debug;
        case webrtc::LS_INFO:
            return Level::Info;
        case webrtc::LS_WARNING:
            return Level::Warning;
        case webrtc::LS_ERROR:
            return Level::Error;
        default:
            return Level::Unknown;
        }
    }

    uint32_t LogSink::parse_line_number(const std::string& message) {
        uint32_t port = -1;
        std::stringstream ss(message);
        ss >> port;
        return port;
    }

    void LogSink::register_log_message(const std::string& message, const webrtc::LoggingSeverity severity) const {
        if (!on_log_message_) {
            return;
        }
        thread_->PostTask([message, severity] {
            if (std::smatch match; std::regex_search(message, match, message_pattern_)) {
                const auto file_name = std::string(match[1]) + "." + std::string(match[2]);
                const auto line_num = parse_line_number(match[3]);
                const auto level = parse_severity(severity);
                const auto parsed_message = std::string(match[4]);
                (void) on_log_message_({
                    level,
                    std::string(match[2]) == "cpp" ? Source::Self : Source::WebRTC,
                    file_name,
                    line_num,
                    parsed_message,
                });
            }
        });
    }

    void LogSink::OnLogMessage(const std::string& msg, const webrtc::LoggingSeverity severity, const char* tag) {
        OnLogMessage(std::string(tag) + ": " + msg, severity);
    }

    void LogSink::OnLogMessage(const std::string& message, const webrtc::LoggingSeverity severity) {
        register_log_message(message, severity);
    }

    void LogSink::OnLogMessage(const std::string& message) {
        register_log_message(message, webrtc::LS_NONE);
    }

    void LogSink::register_logger(std::function<void(LogMessage)> callback) {
        on_log_message_ = std::move(callback);
    }

    void LogSink::get_or_create() {
        const std::lock_guard lock(mutex_);
        references_++;
        if (references_ == 1) {
            instance_ = webrtc::scoped_refptr<LogSink>(new webrtc::RefCountedObject<LogSink>());
        }
    }

    void LogSink::un_ref() {
        const std::lock_guard lock(mutex_);
        references_--;
        if (!references_) {
            instance_ = nullptr;
        }
    }
} // ntgcalls::utils
