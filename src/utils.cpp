//
// Created by Laky64 on 26/07/2023.
//

#include "utils.hpp"


Sdp parseSdp(const std::string& sdp) {
    std::vector<std::string> lines;
    std::string line;
    std::istringstream stream(sdp);
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    auto lookup = [&lines](const std::string& prefix) -> std::string {
        for (const auto& line : lines) {
            if (line.compare(0, prefix.size(), prefix) == 0) {
                return line.substr(prefix.size());
            }
        }
        return "";
    };

    std::string rawAudioSource = lookup("a=ssrc:");
    std::string rawVideoSource = lookup("a=ssrc-group:FID ");
    return {
            lookup("a=fingerprint:").substr(0, lookup("a=fingerprint:").find(' ')),
            lookup("a=fingerprint:").substr(lookup("a=fingerprint:").find(' ') + 1),
            lookup("a=setup:"),
            lookup("a=ice-pwd:"),
            lookup("a=ice-ufrag:"),
            static_cast<uint32_t>(rawAudioSource.empty() ? 0 : std::stoi(rawAudioSource.substr(0, rawAudioSource.find(' ')))),
            rawVideoSource.empty() ? std::vector<uint32_t>() : std::vector<uint32_t>{static_cast<uint32_t>(std::stoi(rawVideoSource.substr(0, rawVideoSource.find(' '))))}
    };
}

uint32_t generateSSRC() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(100000000, 1000000000);
    return dis(gen);
}

int64_t getMilliseconds() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return milliseconds;
}

std::string generateUniqueId(int length) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 9);

    std::stringstream ss;
    for (int i = 0; i < length; ++i) {
        ss << dis(gen);
    }
    return ss.str();
}