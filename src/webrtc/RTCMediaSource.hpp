//
// Created by Laky64 on 03/08/2023.
//

#ifndef NTGCALLS_RTCMEDIASOURCE_HPP
#define NTGCALLS_RTCMEDIASOURCE_HPP


#include <memory>
#include "MediaStreamTrack.hpp"

class RTCMediaSource {
protected:
    std::shared_ptr<MediaStreamTrack> track;
    MediaStreamTrack::Type codec;


public:
    explicit RTCMediaSource(MediaStreamTrack::Type type);

    void addTrack(const std::shared_ptr<rtc::PeerConnection>& pc);

    void onOpen(const std::function<void()> &callback);

    void sendData(rtc::binary samples, uint64_t sampleTime);
};


#endif
