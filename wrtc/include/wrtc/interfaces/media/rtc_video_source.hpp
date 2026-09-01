//
// Created by Lauren on 19/08/23.
//

#pragma once
#include <wrtc/interfaces/media/tracks/video_track_source.hpp>
#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>
#include <wrtc/models/frame_data.hpp>
#include <wrtc/models/i420_image_data.hpp>

namespace wrtc::interfaces::media {

    class RTCVideoSource {
    public:
        RTCVideoSource();

        ~RTCVideoSource();

        [[nodiscard]] webrtc::scoped_refptr<webrtc::VideoTrackInterface> create_track() const;

        void on_frame(const models::I420ImageData& data, models::FrameData additional_data) const;

    private:
        webrtc::scoped_refptr<tracks::VideoTrackSource> source_;
        peer_connection::PeerConnectionFactory* factory_;
    };

} // wrtc::interfaces::media
