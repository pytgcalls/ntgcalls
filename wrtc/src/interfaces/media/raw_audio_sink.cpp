//
// Created by Lauren on 07/10/24.
//

#include <utility>
#include <rtc_base/time_utils.h>
#include <wrtc/interfaces/media/raw_audio_sink.hpp>

namespace wrtc::interfaces::media {
    RawAudioSink::~RawAudioSink() {
        callback_ = nullptr;
    }

    void RawAudioSink::OnData(const Data& audio) {
        if (callback_ && encoded_) {
            const auto packet = encoded_->pop();
            auto frame = std::make_unique<models::AudioFrame>(ssrc_);
            frame->encoded = true;
            if (packet) {
                frame->data = reinterpret_cast<const int16_t*>(packet->payload.data());
                frame->size = packet->payload.size();
            }
            frame->sample_rate = audio.sample_rate;
            frame->channels = audio.channels;
            frame->absolute_capture_timestamp_ms = webrtc::TimeMillis();
            callback_(std::move(frame));
        } else if (callback_) {
            auto frame = std::make_unique<models::AudioFrame>(ssrc_);
            frame->size = audio.samples_per_channel * audio.channels * sizeof(int16_t);
            frame->data = audio.data;
            frame->sample_rate = audio.sample_rate;
            frame->channels = audio.channels;
            frame->absolute_capture_timestamp_ms = webrtc::TimeMillis();
            callback_(std::move(frame));
        }
    }

    void RawAudioSink::set_encoded_queue(std::shared_ptr<utils::encoded::EncodedQueue> queue) {
        encoded_ = std::move(queue);
    }

    void RawAudioSink::set_remote_audio_sink(const uint32_t ssrc, std::function<void(std::unique_ptr<models::AudioFrame>)> callback) {
        ssrc_ = ssrc;
        callback_ = std::move(callback);
    }
} // wrtc::interfaces::media
