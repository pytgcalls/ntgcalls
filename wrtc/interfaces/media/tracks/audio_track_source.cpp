//
// Created by Laky64 on 19/08/2023.
//

#include "audio_track_source.hpp"
#include <rtc_base/time_utils.h>

namespace wrtc {

    AudioTrackSource::~AudioTrackSource() {
        _sink = nullptr;
    }

    webrtc::MediaSourceInterface::SourceState AudioTrackSource::state() const {
        return kLive;
    }

    bool AudioTrackSource::remote() const {
        return false;
    }

    void AudioTrackSource::AddSink(webrtc::AudioTrackSinkInterface *sink) {
        _sink = sink;
    }

    void AudioTrackSource::RemoveSink(webrtc::AudioTrackSinkInterface *) {
        _sink = nullptr;
    }

    void AudioTrackSource::PushData(const RTCOnDataEvent &data, const int64_t absolute_capture_timestamp_ms) const {
        if (webrtc::AudioTrackSinkInterface *sink = _sink) {
            sink->OnData(
                data.audioData.get(),
                data.bitsPerSample,
                static_cast<int>(data.sampleRate),
                data.channelCount,
                data.numberOfFrames,
                absolute_capture_timestamp_ms
            );
        }
    }
} // wrtc