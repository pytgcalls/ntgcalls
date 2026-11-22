//
// Created by Lauren on 07/10/24.
//

#include <wrtc/audio_factory/opus/opus_jitter_buffer.hpp>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>

namespace wrtc::interfaces::media {
    RemoteAudioSink::RemoteAudioSink(const std::function<void(const std::vector<std::unique_ptr<models::AudioFrame>>&)>& callback): num_sources_(0), max_delay_ms_(audio_factory::OpusJitterBuffer::kDefaultMaxDelayMs) {
        frames_callback_ = callback;
    }

    RemoteAudioSink::~RemoteAudioSink() {
        frames_callback_ = nullptr;
        audio_frames_.clear();
    }

    void RemoteAudioSink::send_data(std::unique_ptr<models::AudioFrame> frame) {
        for (const auto& pending_frame : audio_frames_) {
            if (pending_frame->ssrc == frame->ssrc) {
                frames_callback_(audio_frames_);
                audio_frames_.clear();
                break;
            }
        }
        audio_frames_.push_back(std::move(frame));
        if (audio_frames_.size() >= num_sources_) {
            frames_callback_(audio_frames_);
            audio_frames_.clear();
        }
    }

    void RemoteAudioSink::set_encoded(const bool encoded) {
        encoded_ = encoded;
    }

    bool RemoteAudioSink::encoded() const {
        return encoded_;
    }

    void RemoteAudioSink::set_max_delay(const std::optional<uint32_t> max_delay_ms) {
        max_delay_ms_ = max_delay_ms ? static_cast<int64_t>(*max_delay_ms) : audio_factory::OpusJitterBuffer::kDefaultMaxDelayMs;
    }

    int64_t RemoteAudioSink::max_delay_ms() const {
        return max_delay_ms_;
    }

    void RemoteAudioSink::update_audio_source_count(const int count) {
        num_sources_ = count;
    }
} // wrtc::interfaces::media
