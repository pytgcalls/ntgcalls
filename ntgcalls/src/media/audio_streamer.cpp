//
// Created by Lauren on 12/08/23.
//

#include <ntgcalls/media/audio_streamer.hpp>

namespace ntgcalls::media {
    AudioStreamer::AudioStreamer() {
        audio_ = std::make_unique<wrtc::interfaces::media::RTCAudioSource>();
    }

    AudioStreamer::~AudioStreamer() {
        audio_ = nullptr;
    }

    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> AudioStreamer::createTrack() {
        return audio_->create_track();
    }

    void AudioStreamer::sendData(uint8_t* sample, size_t size, const wrtc::models::FrameData additional_data) {
        frames_++;
        auto event = wrtc::models::RTCOnDataEvent(sample, frame_size() / (2 * description_->channel_count));
        event.channel_count = description_->channel_count;
        event.sample_rate = description_->sample_rate;
        event.bits_per_sample = 16;
        audio_->on_data(event, additional_data);
    }
} // ntgcalls::media
