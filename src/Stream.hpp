//
// Created by Laky64 on 03/08/2023.
//

#ifndef NTGCALLS_STREAM_HPP
#define NTGCALLS_STREAM_HPP


#include <fstream>
#include <memory>
#include <utility>
#include "webrtc/RTCAudioSource.hpp"
#include "utils/DispatchQueue.hpp"
#include "utils/time.hpp"
#include "io/FileReader.hpp"

class Stream {
private:
    DispatchQueue dispatchQueue = DispatchQueue("StreamQueue");
    std::shared_ptr<RTCAudioSource> audioSrc;
    std::shared_ptr<BaseReader> audioReader;

    // TEMPORARY VARIABLES
    uint64_t sampleDuration_us, sampleTime_us, startTime = 0;

    void start();

    void processData();

public:
    explicit Stream(std::shared_ptr<BaseReader> audio);

    void addTracks(const std::shared_ptr<rtc::PeerConnection> &pc);
};


#endif //NTGCALLS_STREAM_HPP
