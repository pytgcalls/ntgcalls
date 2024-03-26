//
// Created by Laky64 on 26/03/2024.
//

#pragma once

#include "rtc_base/logging.h"
#include <fstream>

namespace ntgcalls {

    class LogSinkImpl final: public rtc::LogSink {
        std::ofstream _file;
        std::ostringstream _data;
        bool allowWebrtcLogs;

        static std::string severityToString(rtc::LoggingSeverity severity);

    public:
        explicit LogSinkImpl(const std::string &logPath, bool allowWebrtcLogs = false);

        void OnLogMessage(const std::string &msg, rtc::LoggingSeverity severity, const char *tag) override;
        void OnLogMessage(const std::string &message, rtc::LoggingSeverity severity) override;
        void OnLogMessage(const std::string &message) override;

        [[nodiscard]] std::string result() const;
    };

} // ntgcalls
