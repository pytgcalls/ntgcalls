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
        lastTime = getMicroseconds();
        sentBytes += frameSize();
    }

    uint64_t BaseStreamer::waitTime() {
        auto currTime = getMicroseconds();
        uint64_t nextTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(frameTime())).count();
        return std::max<uint64_t>(lastTime - currTime + nextTime, 0);
    }

    void BaseStreamer::clear() {
        sentBytes = 0;
        lastTime = 0;
    }
}
