//
// Created by iraci on 26/07/2023.
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
            rawAudioSource.empty() ? 0 : std::stoi(rawAudioSource.substr(0, rawAudioSource.find(' '))),
            rawVideoSource.empty() ? std::vector<int>() : std::vector<int>{std::stoi(rawVideoSource.substr(0, rawVideoSource.find(' ')))}
    };
}

int generateSSRC() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(100000000, 1000000000);
    return dis(gen);
}