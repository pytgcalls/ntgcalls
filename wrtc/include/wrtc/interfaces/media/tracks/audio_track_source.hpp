//
// Created by Lauren on 19/08/23.
//

#pragma once

#include <pc/local_audio_source.h>

#include <wrtc/models/rtc_on_data_event.hpp>

namespace wrtc::interfaces::media::tracks {

    class AudioTrackSource: public webrtc::LocalAudioSource {
    public:
        AudioTrackSource();

        ~AudioTrackSource() override;

        [[nodiscard]] SourceState state() const override;

        [[nodiscard]] bool remote() const override;

        void AddSink(webrtc::AudioTrackSinkInterface* sink) override;

        void RemoveSink(webrtc::AudioTrackSinkInterface* sink) override;

        void push_data(const models::RTCOnDataEvent&, int64_t absolute_capture_timestamp_ms) const;

    private:
        std::atomic<webrtc::AudioTrackSinkInterface*> sink_ = {nullptr};
    };

} // wrtc::interfaces::media::tracks
