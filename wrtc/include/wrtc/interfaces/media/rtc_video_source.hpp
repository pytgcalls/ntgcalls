//
// Created by Laky64 on 19/08/2023.
//

#pragma once
#include <wrtc/models/frame_data.hpp>
#include <wrtc/models/i420_image_data.hpp>
#include <wrtc/interfaces/media/tracks/video_track_source.hpp>
#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>

namespace wrtc {

    class RTCVideoSource {
    public:
        RTCVideoSource();

        ~RTCVideoSource();

        [[nodiscard]] rtc::scoped_refptr<webrtc::VideoTrackInterface> createTrack() const;

        void OnFrame(const i420ImageData& data, FrameData additionalData) const;

    private:
        rtc::scoped_refptr<VideoTrackSource> source;
        rtc::scoped_refptr<PeerConnectionFactory> factory;
    };

} // wrtc
