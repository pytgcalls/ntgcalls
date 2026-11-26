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
        const auto description = this->description();
        if (!description) {
            return;
        }
        if (description->is_encoded()) {
            send_encoded(sample, size, description->channel_count, additional_data);
            return;
        }
        ++frames_;
        send_pcm(sample, description->sample_rate, description->channel_count, additional_data);
    }

    void AudioStreamer::send_pcm(uint8_t* sample, const uint32_t sample_rate, const uint8_t channel_count, const wrtc::models::FrameData &additional_data) const {
        auto event = wrtc::models::RTCOnDataEvent(sample, sample_rate / 100);
        event.channel_count = channel_count;
        event.sample_rate = sample_rate;
        event.bits_per_sample = 16;
        audio_->on_data(event, additional_data);
    }

    void AudioStreamer::send_encoded(const uint8_t* packet, const size_t size, const uint8_t channel_count, const wrtc::models::FrameData &additional_data) {
        const auto frames = wrtc::audio_factory::OpusPacket::frames(packet, size);
        if (frames == 0) {
            return;
        }
        encoded_->push({{packet, packet + size}, additional_data});
        if (!level_) {
            level_ = std::make_unique<wrtc::audio_factory::OpusLevel>(channel_count);
        }
        level_frame_.resize(static_cast<size_t>(wrtc::audio_factory::OpusPacket::kFrameSamples) * channel_count * sizeof(int16_t));
        fill_level(level_frame_, channel_count, level_->amplitude(packet, size));
        for (size_t i = 0; i < frames; i++) {
            ++frames_;
            send_pcm(level_frame_.data(), wrtc::audio_factory::OpusPacket::kSampleRate, channel_count, additional_data);
        }
    }

    void AudioStreamer::fill_level(bytes::binary& frame, const size_t channels, const int16_t amplitude) {
        const auto samples = reinterpret_cast<int16_t*>(frame.data());
        for (size_t i = 0; i < frame.size() / sizeof(int16_t); i++) {
            samples[i] = i / channels % 2 ? static_cast<int16_t>(-amplitude) : amplitude;
        }
    }

    std::shared_ptr<wrtc::utils::encoded::EncodedQueue> AudioStreamer::encoded_queue() const {
        return encoded_;
    }
} // ntgcalls::media
