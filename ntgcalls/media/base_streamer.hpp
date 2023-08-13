//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <wrtc/wrtc.hpp>
#include <cstdint>

#include "../utils/time.hpp"
#include "../exceptions.hpp"


namespace ntgcalls {
    class BaseStreamer {
    private:
        uint64_t sentBytes, lastSentTime = 0;

    protected:
        ~BaseStreamer();

        virtual uint64_t frameTime() = 0;

        void clear();

    public:
        uint64_t time();

        uint64_t waitTime();

        virtual wrtc::MediaStreamTrack *createTrack() = 0;

        virtual void sendData(wrtc::binary sample);

        virtual uint64_t frameSize() = 0;
    };
}
