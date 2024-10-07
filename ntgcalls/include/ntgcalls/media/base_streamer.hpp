//
// Created by Laky64 on 28/09/24.
//

#pragma once

#include <api/media_stream_interface.h>

namespace ntgcalls {

    class BaseStreamer {
    public:
        virtual ~BaseStreamer() = default;

        virtual void sendData(uint8_t* sample, int64_t absolute_capture_timestamp_ms) = 0;

        virtual rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> createTrack() = 0;
    };

} // ntgcalls
