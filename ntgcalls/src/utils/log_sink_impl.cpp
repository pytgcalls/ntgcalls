//
// Created by Laky64 on 26/03/2024.
//

#include <ntgcalls/utils/log_sink_impl.hpp>

#include <regex>
#include <rtc_base/ref_counted_object.h>

namespace ntgcalls {
    rtc::scoped_refptr<LogSink> LogSink::instance = nullptr;
    std::mutex LogSink::mutex{};
    uint32_t LogSink::references = 0;
#ifndef PYTHON_ENABLED
    wrtc::synchronized_callback<LogSink::LogMessage> LogSink::onLogMessage{};
#endif

    LogSink::LogSink() {
        thread = rtc::Thread::Create();
        thread->SetName("LogSink", nullptr);
        thread->Start();
#ifdef DEBUG
        rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
#else
        rtc::LogMessage::LogToDebug(rtc::LS_INFO);
#endif
        rtc::LogMessage::SetLogToStderr(false);
        rtc::LogMessage::AddLogToStream(this, rtc::LS_VERBOSE);
#ifdef PYTHON_ENABLED
        THREAD_SAFE
        const auto loggingLib = py::module::import("logging");
        rtcLogs = loggingLib.attr("getLogger")("webrtc");
        if (rtcLogs.attr("level").equal(loggingLib.attr("NOTSET"))) {
            rtcLogs.attr("setLevel")(loggingLib.attr("CRITICAL"));
        }
        ntgLogs = loggingLib.attr("getLogger")("ntgcalls");
        if (ntgLogs.attr("level").equal(loggingLib.attr("NOTSET"))) {
            ntgLogs.attr("setLevel")(loggingLib.attr("CRITICAL"));
        }
        END_THREAD_SAFE
#endif
    }

    LogSink::~LogSink() {
        rtc::LogMessage::RemoveLogToStream(this);
        thread->Stop();
        thread = nullptr;
    }

#ifdef PYTHON_ENABLED
    py::object LogSink::parseSeverity(const rtc::LoggingSeverity severity) {
        THREAD_SAFE
        const auto loggingLib = py::module::import("logging");
        switch (severity) {
            case rtc::LS_VERBOSE:
                return loggingLib.attr("DEBUG");
            case rtc::LS_INFO:
                return loggingLib.attr("INFO");
            case rtc::LS_WARNING:
                return loggingLib.attr("WARNING");
            case rtc::LS_ERROR:
                return loggingLib.attr("ERROR");
            default:
                return loggingLib.attr("NOTSET");
        }
        END_THREAD_SAFE
    }
#else
    LogSink::Level LogSink::parseSeverity(const rtc::LoggingSeverity severity) {
        switch (severity) {
            case rtc::LS_VERBOSE:
                return Level::Debug;
            case rtc::LS_INFO:
                return Level::Info;
            case rtc::LS_WARNING:
                return Level::Warning;
            case rtc::LS_ERROR:
                return Level::Error;
            default:
                return Level::Unknown;
        }
    }
#endif

    uint32_t LogSink::parseLineNumber(const std::string &message) {
        uint32_t port = -1;
        std::stringstream ss(message);
        ss >> port;
        return port;
    }

    void LogSink::registerLogMessage(const std::string &message, const rtc::LoggingSeverity severity) const {
        thread->PostTask([this, message, severity] {
#ifdef PYTHON_ENABLED
            if (!Py_IsInitialized()) {
                return;
            }
#endif
            const std::regex regex(R"(\((.*)\.(.*):([0-9]+)\):\s?(.*))");
            if (std::smatch match; std::regex_search(message, match, regex)) {
                const auto fileName = std::string(match[1]) + "." + std::string(match[2]);
                const auto lineNum = parseLineNumber(match[3]);
                const auto level = parseSeverity(severity);
                const auto parsedMessage = std::string(match[4]);
#ifdef PYTHON_ENABLED
                const auto logMess = fileName + ":" + std::to_string(lineNum) + " " + parsedMessage;
                THREAD_SAFE
                if (match[2] == "cpp") {
                    (void) ntgLogs.attr("log")(level, logMess);
                } else {
                    (void) rtcLogs.attr("log")(level, logMess);
                }
                END_THREAD_SAFE
#else
                (void) onLogMessage({
                    level,
                    std::string(match[2]) == "cpp" ? Source::Self : Source::WebRTC,
                    fileName,
                    lineNum,
                    parsedMessage
                });
#endif
            }
        });
    }

    void LogSink::OnLogMessage(const std::string& msg, const rtc::LoggingSeverity severity, const char* tag) {
        OnLogMessage(std::string(tag) + ": " + msg, severity);
    }

    void LogSink::OnLogMessage(const std::string& message, const rtc::LoggingSeverity severity) {
        registerLogMessage(message, severity);
    }

    void LogSink::OnLogMessage(const std::string& message) {
        registerLogMessage(message, rtc::LS_NONE);
    }

#ifndef PYTHON_ENABLED
    void LogSink::registerLogger(std::function<void(LogMessage)> callback) {
        onLogMessage = std::move(callback);
    }
#endif

    void LogSink::GetOrCreate() {
        std::lock_guard lock(mutex);
        references++;
        if (references == 1) {
            instance = rtc::scoped_refptr<LogSink>(new rtc::RefCountedObject<LogSink>());
        }
    }

    void LogSink::UnRef() {
        std::lock_guard lock(mutex);
        references--;
        if (!references) {
            instance = nullptr;
        }
    }
} // ntgcalls