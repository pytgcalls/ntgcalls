//
// Created by Laky64 on 12/08/2023.
//

#include "base_streamer.hpp"

namespace ntgcalls {
    BaseStreamer::~BaseStreamer() {
        clear();
    }

    void BaseStreamer::sendData(wrtc::binary sample) {
        lastTime = getMicroseconds();
        sentBytes += frameSize();
    }

    uint64_t BaseStreamer::time() {
        return (sentBytes / frameSize()) / frameTime();
    }

    uint64_t BaseStreamer::waitTime() {
        int64_t currTime = getMicroseconds();
        int64_t nextTime = frameTime() * 1000.0;
        return std::max<int64_t>(lastTime - currTime + nextTime, 0);
    }

    void BaseStreamer::clear() {
        sentBytes = 0;
        lastTime = 0;
    }
}
