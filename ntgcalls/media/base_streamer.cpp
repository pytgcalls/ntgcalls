//
// Created by Laky64 on 12/08/2023.
//

#include "base_streamer.hpp"

namespace ntgcalls {
    BaseStreamer::~BaseStreamer() {
        clear();
    }

    void BaseStreamer::sendData(const wrtc::binary& sample) {
        lastTime = std::chrono::high_resolution_clock::now();
        sentFrames++;
    }

    uint64_t BaseStreamer::time() {
        return std::chrono::duration_cast<std::chrono::seconds>(nanoTime()).count();
    }

    std::chrono::nanoseconds BaseStreamer::nanoTime() {
        return sentFrames * frameTime();
    }

    std::chrono::nanoseconds BaseStreamer::waitTime() {
        return lastTime - std::chrono::high_resolution_clock::now() + frameTime();
    }

    void BaseStreamer::clear() {
        sentFrames = 0;
    }
}
