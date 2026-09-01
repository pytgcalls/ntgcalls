//
// Created by Lauren on 19/08/23.
//

#include <wrtc/interfaces/media/tracks/audio_track_source.hpp>

namespace wrtc::interfaces::media::tracks {

    AudioTrackSource::AudioTrackSource(): LocalAudioSource(nullptr) {}

    AudioTrackSource::~AudioTrackSource() {
        sink_ = nullptr;
    }

    webrtc::MediaSourceInterface::SourceState AudioTrackSource::state() const {
        return kLive;
    }

    bool AudioTrackSource::remote() const {
        return false;
    }

    void AudioTrackSource::AddSink(webrtc::AudioTrackSinkInterface* sink) {
        sink_ = sink;
    }

    void AudioTrackSource::RemoveSink(webrtc::AudioTrackSinkInterface*) {
        sink_ = nullptr;
    }

    void AudioTrackSource::push_data(const models::RTCOnDataEvent& data, const int64_t absolute_capture_timestamp_ms) const {
        if (webrtc::AudioTrackSinkInterface* sink = sink_) {
            sink->OnData(
                data.audio_data,
                data.bits_per_sample,
                static_cast<int>(data.sample_rate),
                data.channel_count,
                data.number_of_frames,
                absolute_capture_timestamp_ms
            );
        }
    }
} // wrtc::interfaces::media::tracks
