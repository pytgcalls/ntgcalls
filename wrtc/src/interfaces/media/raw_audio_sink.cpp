//
// Created by Lauren on 07/10/24.
//


#include <utility>
#include <rtc_base/time_utils.h>
#include <wrtc/interfaces/media/raw_audio_sink.hpp>

namespace wrtc::interfaces::media {
    void RawAudioSink::OnData(const Data& audio) {
        const auto remote_sink = remote_sink_.lock();
        if (!remote_sink) {
            return;
        }
        if (!encoded_) {
            auto frame = std::make_unique<models::AudioFrame>(ssrc_);
            frame->size = audio.samples_per_channel * audio.channels * sizeof(int16_t);
            frame->data = audio.data;
            frame->sample_rate = audio.sample_rate;
            frame->channels = audio.channels;
            frame->absolute_capture_timestamp_ms = webrtc::TimeMillis();
            remote_sink->send_data(std::move(frame));
            return;
        }
        while (auto packet = encoded_->pop()) {
            jitter_buffer_.push(std::move(*packet));
        }
        const auto packets = jitter_buffer_.pop(webrtc::TimeMillis(), remote_sink->max_delay_ms());
        if (packets.empty()) {
            send_encoded(remote_sink, audio, nullptr);
        }
        for (const auto& packet : packets) {
            send_encoded(remote_sink, audio, &packet);
        }
    }

    void RawAudioSink::send_encoded(const std::shared_ptr<RemoteAudioSink>& remote_sink, const Data& audio, const utils::encoded::EncodedFrame* packet) const {
        auto frame = std::make_unique<models::AudioFrame>(ssrc_);
        frame->encoded = true;
        frame->absolute_capture_timestamp_ms = webrtc::TimeMillis();
        if (packet) {
            frame->data = reinterpret_cast<const int16_t*>(packet->payload.data());
            frame->size = packet->payload.size();
            frame->absolute_capture_timestamp_ms = packet->frame_data.absolute_capture_timestamp_ms;
        }
        frame->sample_rate = audio.sample_rate;
        frame->channels = audio.channels;
        remote_sink->send_data(std::move(frame));
    }

    void RawAudioSink::set_encoded_queue(std::shared_ptr<utils::encoded::EncodedQueue> queue) {
        encoded_ = std::move(queue);
    }

    void RawAudioSink::set_remote_audio_sink(const uint32_t ssrc, std::weak_ptr<RemoteAudioSink> remote_sink) {
        ssrc_ = ssrc;
        remote_sink_ = std::move(remote_sink);
    }
} // wrtc::interfaces::media
