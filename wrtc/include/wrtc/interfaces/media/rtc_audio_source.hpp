//
// Created by Lauren on 19/08/23.
//

#pragma once

#include <wrtc/interfaces/media/tracks/audio_track_source.hpp>
#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>
#include <wrtc/models/frame_data.hpp>
#include <wrtc/models/rtc_on_data_event.hpp>

namespace wrtc::interfaces::media {

    class RTCAudioSource {
    public:
        RTCAudioSource();

        ~RTCAudioSource();

        [[nodiscard]] webrtc::scoped_refptr<webrtc::AudioTrackInterface> create_track() const;

        void on_data(const models::RTCOnDataEvent &, models::FrameData additional_data) const;

    private:
        webrtc::scoped_refptr<tracks::AudioTrackSource> source_;
        peer_connection::PeerConnectionFactory* factory_;
    };

} // wrtc::interfaces::media
