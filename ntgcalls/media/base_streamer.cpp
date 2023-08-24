//
// Created by Laky64 on 12/08/2023.
//

#include "base_streamer.hpp"

namespace ntgcalls {
    BaseStreamer::~BaseStreamer() {
        clear();
    }

    void BaseStreamer::sendData(wrtc::binary sample) {
        lastTime = std::chrono::high_resolution_clock::now();
        sentBytes += frameSize();
    }

    uint64_t BaseStreamer::time() {
        return (sentBytes / frameSize()) / frameTime();
    }

    std::chrono::nanoseconds BaseStreamer::waitTime() {
        auto diffTime = std::chrono::duration_cast<std::chrono::nanoseconds>(lastTime - std::chrono::high_resolution_clock::now());
        auto nextTime = std::chrono::nanoseconds(int64_t(frameTime() * 1000.0 * 1000.0));
        return diffTime + nextTime;
    }

    void BaseStreamer::clear() {
        sentBytes = 0;
    }
}
