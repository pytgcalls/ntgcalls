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

        virtual void send_data(uint8_t* sample, size_t size, wrtc::models::FrameData additional_data) = 0;

        virtual webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> create_track() = 0;
    };

} // ntgcalls
