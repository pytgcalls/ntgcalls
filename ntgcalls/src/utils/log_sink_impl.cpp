//
// Created by Lauren on 26/03/24.
//

#include <ntgcalls/utils/log_sink_impl.hpp>

#include <charconv>
#include <rtc_base/ref_counted_object.h>

namespace ntgcalls::utils {
    webrtc::scoped_refptr<LogSink> LogSink::instance_ = nullptr;
    std::mutex LogSink::mutex_{};
    uint32_t LogSink::references_ = 0;
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

    bool LogSink::parse_message(const std::string& message, std::string& file_name, std::string& extension, uint32_t& line_number, std::string& body) {
        const auto view = std::string_view(message);
        const auto open = view.find('(');
        if (open == std::string_view::npos) {
            return false;
        }
        const auto close = view.find("):", open);
        if (close == std::string_view::npos) {
            return false;
        }
        const auto header = view.substr(open + 1, close - open - 1);
        const auto colon = header.rfind(':');
        if (colon == std::string_view::npos) {
            return false;
        }
        const auto dot = header.rfind('.', colon);
        if (dot == std::string_view::npos) {
            return false;
        }
        const auto digits = header.substr(colon + 1);
        if (digits.empty()) {
            return false;
        }
        if (const auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), line_number); ec != std::errc() || ptr != digits.data() + digits.size()) {
            return false;
        }
        file_name = std::string(header.substr(0, colon));
        extension = std::string(header.substr(dot + 1, colon - dot - 1));
        auto rest = view.substr(close + 2);
        if (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
            rest.remove_prefix(1);
        }
        if (const auto end_of_line = rest.find('\n'); end_of_line != std::string_view::npos) {
            rest = rest.substr(0, end_of_line);
        }
        body = std::string(rest);
        return true;
    }

    void LogSink::register_log_message(const std::string& message, const webrtc::LoggingSeverity severity) const {
        if (!on_log_message_) {
            return;
        }
        thread_->PostTask([message, severity] {
            std::string file_name;
            std::string extension;
            std::string parsed_message;
            uint32_t line_num = 0;
            if (!parse_message(message, file_name, extension, line_num, parsed_message)) {
                return;
            }
            (void) on_log_message_({
                parse_severity(severity),
                extension == "cpp" ? Source::Self : Source::WebRTC,
                file_name,
                line_num,
                parsed_message,
            });
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
