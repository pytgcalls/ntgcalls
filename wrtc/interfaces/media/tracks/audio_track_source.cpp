//
// Created by Laky64 on 19/08/2023.
//

#include "audio_track_source.hpp"

namespace wrtc {

    AudioTrackSource::~AudioTrackSource() {
        _sink = nullptr;
    }

    webrtc::MediaSourceInterface::SourceState AudioTrackSource::state() const {
        return webrtc::MediaSourceInterface::SourceState::kLive;
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

    void AudioTrackSource::PushData(RTCOnDataEvent &data) {
        webrtc::AudioTrackSinkInterface *sink = _sink;
        if (sink) {
            sink->OnData(
                    data.audioData,
                    data.bitsPerSample,
                    data.sampleRate,
                    data.channelCount,
                    data.numberOfFrames
            );
        }
    }
} // wrtc