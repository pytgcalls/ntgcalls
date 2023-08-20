//
// Created by Laky64 on 12/08/2023.
//

#include "base_streamer.hpp"

namespace ntgcalls {
    BaseStreamer::~BaseStreamer() {
        clear();
    }

    uint64_t BaseStreamer::time() {
        return (sentBytes / frameSize()) / frameTime();
    }

    void BaseStreamer::sendData(wrtc::binary sample) {
        lastSentTime = getMicroseconds();
        sentBytes += frameSize();
    }

    uint64_t BaseStreamer::waitTime() {
        auto currentTime = getMicroseconds();
        auto nextTime = frameTime() * 1000; // Microseconds
        return std::max<uint64_t>(nextTime - currentTime - lastSentTime, 0);
    }

    void BaseStreamer::clear() {
        sentBytes = 0;
        lastSentTime = 0;
    }
}
