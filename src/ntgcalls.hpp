//
// Created by Laky64 on 29/07/2023.
//

#ifndef NTGCALLS_NTGCALLS_HPP
#define NTGCALLS_NTGCALLS_HPP

#include <iostream>
#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>
#include "webrtc/SdpBuilder.hpp"
#include "Stream.hpp"

using nlohmann::json;

struct JoinVoiceCallParams {
    std::string ufrag;
    std::string pwd;
    std::string hash;
    std::string setup;
    std::string fingerprint;
};

class NTgCalls
{
private:
    std::shared_ptr<rtc::PeerConnection> connection;
    uint32_t audioSource;
    std::vector<uint32_t> sourceGroups;
    std::shared_ptr<Stream> stream;

    std::optional<JoinVoiceCallParams> init(const std::shared_ptr<Stream> &mediaStream);

public:
    std::string createCall();
    void setRemoteCallParams(const std::string& jsonData);
};

#endif