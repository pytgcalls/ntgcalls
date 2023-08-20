//
// Created by Laky64 on 19/08/2023.
//

#pragma once

#include <pc/local_audio_source.h>

#include "../../../models/rtc_on_data_event.hpp"
#include "../../peer_connection/peer_connection_factory.hpp"

namespace wrtc {

    class AudioTrackSource: public webrtc::LocalAudioSource {
    public:
        ~AudioTrackSource() override;

        SourceState state() const override;

        bool remote() const override;

        void AddSink(webrtc::AudioTrackSinkInterface *sink) override;

        void RemoveSink(webrtc::AudioTrackSinkInterface *sink) override;

        void PushData(RTCOnDataEvent &);

    private:
        std::atomic<webrtc::AudioTrackSinkInterface *> _sink = {nullptr};
    };

} // wrtc
