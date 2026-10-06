//
// Created by Lauren on 07/10/24.
//

#include <wrtc/interfaces/media/remote_audio_sink.hpp>

namespace wrtc::interfaces::media {
    RemoteAudioSink::RemoteAudioSink(const std::function<void(const std::vector<std::unique_ptr<models::AudioFrame>>&)>& callback): num_sources_(0) {
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

    void RemoteAudioSink::update_audio_source_count(const int count) {
        num_sources_ = count;
    }
} // wrtc::interfaces::media
