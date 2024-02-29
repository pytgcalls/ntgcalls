//
// Created by Laky64 on 19/08/2023.
//

#pragma once

#include <api/scoped_refptr.h>

#include "tracks/audio_track_source.hpp"
#include "tracks/media_stream_track.hpp"
#include "../peer_connection/peer_connection_factory.hpp"
#include "../../models/rtc_on_data_event.hpp"

namespace wrtc {

    class RTCAudioSource {
    public:
        RTCAudioSource();

        ~RTCAudioSource();

        [[nodiscard]] MediaStreamTrack *createTrack() const;

        void OnData(const RTCOnDataEvent &, int64_t absolute_capture_timestamp_ms) const;

    private:
        rtc::scoped_refptr<AudioTrackSource> source;
        rtc::scoped_refptr<PeerConnectionFactory> factory;
    };

} // wrtc
