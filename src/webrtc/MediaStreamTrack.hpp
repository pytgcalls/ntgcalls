//
// Created by Laky64 on 03/08/2023.
//

#ifndef NTGCALLS_MEDIASTREAMTRACK_HPP
#define NTGCALLS_MEDIASTREAMTRACK_HPP


#include <string>
#include <random>
#include <sstream>
#include <memory>
#include "rtc/rtc.hpp"

class MediaStreamTrack {
private:
    std::string cname, trackId, msid, mid;
    uint32_t ssrc;
    std::shared_ptr<rtc::Track> track;
    std::shared_ptr<rtc::RtcpSrReporter> srReporter;

    static std::string generateUniqueId(
            int length,
            const std::string& chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+"
    );

    static std::string generateTrackId();

    static uint32_t generateSSRC();

public:
    enum Type {
        Audio = 111,
        Video = 102
    };

    explicit MediaStreamTrack(Type codec, const std::shared_ptr<rtc::PeerConnection>& pc);

    ~MediaStreamTrack();

    void onOpen(const std::function<void()> &callback);

    void sendData(rtc::binary samples, uint64_t sampleTime);
};


#endif
