//
// Created by Laky64 on 03/08/2023.
//

#ifndef NTGCALLS_STREAM_HPP
#define NTGCALLS_STREAM_HPP


#include <fstream>
#include <memory>
#include "webrtc/RTCAudioSource.hpp"
#include "utils/DispatchQueue.hpp"
#include "utils/time.hpp"

class Stream {
private:
    std::shared_ptr<RTCAudioSource> audioSrc;

    // TEMPORARY VARIABLES
    DispatchQueue dispatchQueue = DispatchQueue("StreamQueue");
    uint64_t sampleDuration_us;
    uint64_t sampleTime_us = 0;
    uint32_t counter = -1;
    rtc::binary sample = {};
    bool isRunning = false;
    uint64_t startTime = 0;
    std::mutex mutex;

    void start();

public:
    Stream();

    void loadNextSample();

    void unsafePrepareForSample();

    void sendSample();

    void addTracks(const std::shared_ptr<rtc::PeerConnection> &pc);
};


#endif //NTGCALLS_STREAM_HPP
