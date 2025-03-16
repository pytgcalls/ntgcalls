//
// Created by Laky64 on 19/08/2023.
//

#pragma once

#include <pc/local_audio_source.h>

#include <wrtc/models/rtc_on_data_event.hpp>

namespace wrtc {

    class AudioTrackSource: public webrtc::LocalAudioSource {
    public:
        ~AudioTrackSource() override;

        [[nodiscard]] SourceState state() const override;

        [[nodiscard]] bool remote() const override;

        void AddSink(webrtc::AudioTrackSinkInterface *sink) override;

        void RemoveSink(webrtc::AudioTrackSinkInterface *sink) override;

        void PushData(const RTCOnDataEvent &, int64_t absoluteCaptureTimestampMs) const;

    private:
        std::atomic<webrtc::AudioTrackSinkInterface *> _sink = {nullptr};
    };

} // wrtc
