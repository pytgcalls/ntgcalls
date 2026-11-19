//
// Created by Lauren on 12/08/23.
//

#include <ntgcalls/media/audio_streamer.hpp>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace ntgcalls::media {
    AudioStreamer::AudioStreamer() {
        audio_ = std::make_unique<wrtc::interfaces::media::RTCAudioSource>();
        encoded_ = std::make_shared<wrtc::utils::encoded::EncodedQueue>();
    }

    AudioStreamer::~AudioStreamer() {
        audio_ = nullptr;
    }

    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> AudioStreamer::create_track() {
        return audio_->create_track();
    }

    void AudioStreamer::send_data(uint8_t* sample, const size_t size, const wrtc::models::FrameData additional_data) {
        if (description_->is_encoded()) {
            send_encoded(sample, size, additional_data);
            return;
        }
        frames_++;
        send_pcm(sample, description_->sample_rate, additional_data);
    }

    void AudioStreamer::send_pcm(uint8_t* sample, const uint32_t sample_rate, const wrtc::models::FrameData& additional_data) const {
        auto event = wrtc::models::RTCOnDataEvent(sample, sample_rate / 100);
        event.channel_count = description_->channel_count;
        event.sample_rate = sample_rate;
        event.bits_per_sample = 16;
        audio_->on_data(event, additional_data);
    }

    void AudioStreamer::send_encoded(const uint8_t* packet, const size_t size, const wrtc::models::FrameData& additional_data) {
        const auto frames = wrtc::audio_factory::OpusPacket::frames(packet, size);
        if (frames == 0) {
            return;
        }
        encoded_->push({{packet, packet + size}, additional_data});
        silence_.resize(static_cast<size_t>(wrtc::audio_factory::OpusPacket::kFrameSamples) * description_->channel_count * sizeof(int16_t));
        for (size_t i = 0; i < frames; i++) {
            frames_++;
            send_pcm(silence_.data(), wrtc::audio_factory::OpusPacket::kSampleRate, additional_data);
        }
    }

    std::shared_ptr<wrtc::utils::encoded::EncodedQueue> AudioStreamer::encoded_queue() const {
        return encoded_;
    }
} // ntgcalls::media
