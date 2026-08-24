//
// Created by Lauren on 07/10/24.
//

#include <utility>
#include <wrtc/interfaces/media/raw_audio_sink.hpp>

namespace wrtc::interfaces::media {
    RawAudioSink::~RawAudioSink() {
        callback_ = nullptr;
    }

    void RawAudioSink::OnData(const Data& audio) {
        if (callback_) {
            auto frame = std::make_unique<models::AudioFrame>(ssrc_);
            frame->size = audio.samples_per_channel * audio.channels * sizeof(int16_t);
            frame->data = audio.data;
            frame->sample_rate = audio.sample_rate;
            frame->channels = audio.channels;
            callback_(std::move(frame));
        }
    }

    void RawAudioSink::set_remote_audio_sink(const uint32_t ssrc, std::function<void(std::unique_ptr<models::AudioFrame>)> callback) {
        ssrc_ = ssrc;
        callback_ = std::move(callback);
    }
} // wrtc::interfaces::media
