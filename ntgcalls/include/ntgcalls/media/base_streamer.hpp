//
// Created by Lauren on 28/09/24.
//

#pragma once

#include <api/media_stream_interface.h>
#include <wrtc/models/frame_data.hpp>

namespace ntgcalls::media {

    class BaseStreamer {
    public:
        virtual ~BaseStreamer() = default;

        virtual void sendData(uint8_t* sample, size_t size, wrtc::models::FrameData additionalData) = 0;

        virtual webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> createTrack() = 0;
    };

} // ntgcalls
