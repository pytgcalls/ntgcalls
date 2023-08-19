//
// Created by Laky64 on 19/08/2023.
//

#pragma once

#include <rtc_base/ref_count.h>

#include "tracks/video_track_source.hpp"
#include "tracks/media_stream_track.hpp"
#include "../../models/i420_image_data.hpp"
#include "../peer_connection/peer_connection_factory.hpp"

namespace wrtc {

    class RTCVideoSource {
    public:
        RTCVideoSource();

        ~RTCVideoSource();

        MediaStreamTrack *createTrack();

        void OnFrame(i420ImageData data);

    private:
        rtc::scoped_refptr<VideoTrackSource> source;
        rtc::scoped_refptr<PeerConnectionFactory> factory;
    };

} // wrtc
