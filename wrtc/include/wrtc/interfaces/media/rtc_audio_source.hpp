//
// Created by Laky64 on 19/08/2023.
//

#pragma once

#include <wrtc/interfaces/media/tracks/audio_track_source.hpp>
#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>
#include <wrtc/models/frame_data.hpp>
#include <wrtc/models/rtc_on_data_event.hpp>

namespace wrtc {

    class RTCAudioSource {
    public:
        RTCAudioSource();

        ~RTCAudioSource();

        [[nodiscard]] rtc::scoped_refptr<webrtc::AudioTrackInterface> createTrack() const;

        void OnData(const RTCOnDataEvent &, FrameData additionalData) const;

    private:
        rtc::scoped_refptr<AudioTrackSource> source;
        rtc::scoped_refptr<PeerConnectionFactory> factory;
    };

} // wrtc
