//
// Created by Laky64 on 19/08/2023.
//

#pragma once

#include <api/scoped_refptr.h>
#include <rtc_base/helpers.h>
#include <rtc_base/ref_count.h>

#include "tracks/audio_track_source.hpp"
#include "tracks/media_stream_track.hpp"
#include "../peer_connection/peer_connection_factory.hpp"
#include "../../models/rtc_on_data_event.hpp"

namespace wrtc {

    class RTCAudioSource {
    public:
        RTCAudioSource();

        ~RTCAudioSource();

        MediaStreamTrack *createTrack();

        void OnData(RTCOnDataEvent &);

    private:
        rtc::scoped_refptr<AudioTrackSource> source;
        rtc::scoped_refptr<PeerConnectionFactory> factory;
    };

} // wrtc
