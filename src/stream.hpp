//
// Created by Laky64 on 28/07/23.
//

#ifndef NTGCALLS_STREAM_H
#define NTGCALLS_STREAM_H

#include "rtc/rtc.hpp"
#include "MediaDescription.hpp"

class Stream{
private:
    Stream() = default;
    std::shared_ptr<MediaHandler> source;

    static std::shared_ptr<MediaHandler> initAudio();
    static std::shared_ptr<MediaHandler> initVideo();
    void startStreaming() const;

public:

    static Stream Audio();
    static Stream Video();
    std::shared_ptr<rtc::Track> addTrack(const std::shared_ptr<rtc::PeerConnection>& pc) const;
};

#endif