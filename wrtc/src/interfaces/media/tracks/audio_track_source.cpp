//
// Created by Laky64 on 19/08/2023.
//

#include <wrtc/interfaces/media/tracks/audio_track_source.hpp>

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

    void AudioTrackSource::PushData(const RTCOnDataEvent &data, const int64_t absoluteCaptureTimestampMs) const {
        if (webrtc::AudioTrackSinkInterface *sink = _sink) {
            sink->OnData(
                data.audioData,
                data.bitsPerSample,
                static_cast<int>(data.sampleRate),
                data.channelCount,
                data.numberOfFrames,
                absoluteCaptureTimestampMs
            );
        }
    }
} // wrtc