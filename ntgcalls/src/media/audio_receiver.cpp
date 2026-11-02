//
// Created by Lauren on 07/10/24.
//

#include <ntgcalls/media/audio_receiver.hpp>
#include <ntgcalls/exceptions.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::media {
    AudioReceiver::~AudioReceiver() {
        const std::lock_guard lock(mutex_);
        sink_ = nullptr;
        resamplers_.clear();
        frames_callback_ = nullptr;
    }

    bytes::unique_binary AudioReceiver::resample_frame(const uint32_t ssrc, bytes::unique_binary data, const size_t size, const uint8_t channels, const uint16_t sample_rate) {
        bytes::unique_binary converted_data;
        size_t pre_sample_size;
        if (channels != description_->channel_count) {
            switch (channels) {
            case 1:
                converted_data = mono_to_stereo(data, size, &pre_sample_size);
                break;
            case 2:
                converted_data = stereo_to_mono(data, size, &pre_sample_size);
                break;
            default:
                RTC_LOG(LS_ERROR) << "Unsupported audio channels count: " << std::to_string(channels);
                throw InvalidParams("Unsupported audio channels count: " + std::to_string(channels));
            }
        } else {
            pre_sample_size = size;
            converted_data = std::move(data);
        }
        const size_t new_size = frame_size();
        auto new_frame = bytes::make_unique_binary(new_size);
        if (description_->sample_rate == sample_rate) {
            std::memcpy(new_frame.get(), converted_data.get(), pre_sample_size);
        } else {
            const size_t src_samples_per_channel = pre_sample_size / sizeof(int16_t) / description_->channel_count;
            if (src_samples_per_channel != static_cast<size_t>(sample_rate / 100)) {
                RTC_LOG(LS_ERROR) << "Unsupported audio frame duration: " << src_samples_per_channel << " samples at " << sample_rate << "Hz";
                throw InvalidParams("Failed to resample audio frame");
            }
            auto& resampler = resamplers_[ssrc];
            if (!resampler) {
                resampler = std::make_unique<webrtc::PushResampler<int16_t>>();
            }
            resampler->Resample(
                webrtc::InterleavedView<const int16_t>(reinterpret_cast<const int16_t*>(converted_data.get()), src_samples_per_channel, description_->channel_count),
                webrtc::InterleavedView<int16_t>(reinterpret_cast<int16_t*>(new_frame.get()), new_size / sizeof(int16_t) / description_->channel_count, description_->channel_count)
            );
        }
        return std::move(new_frame);
    }

    bytes::unique_binary AudioReceiver::stereo_to_mono(const bytes::unique_binary& data, const size_t size, size_t* new_size) {
        *new_size = size / 2;
        auto mono_data = bytes::make_unique_binary(*new_size);
        for (size_t i = 0; i < size / sizeof(int16_t); i += 2) {
            const auto left = reinterpret_cast<int16_t*>(data.get())[i];
            const auto right = reinterpret_cast<int16_t*>(data.get())[i + 1];
            const auto sum = static_cast<int32_t>(left) + static_cast<int32_t>(right);
            reinterpret_cast<int16_t*>(mono_data.get())[i / 2] = static_cast<int16_t>(sum / 2);
        }
        return std::move(mono_data);
    }

    bytes::unique_binary AudioReceiver::mono_to_stereo(const bytes::unique_binary& data, const size_t size, size_t* new_size) {
        *new_size = size * 2;
        auto stereo_data = bytes::make_unique_binary(*new_size);
        for (size_t i = 0; i < size / sizeof(int16_t); i++) {
            const auto sample = reinterpret_cast<int16_t*>(data.get())[i];
            reinterpret_cast<int16_t*>(stereo_data.get())[i * 2] = sample;
            reinterpret_cast<int16_t*>(stereo_data.get())[i * 2 + 1] = sample;
        }
        return std::move(stereo_data);
    }

    void AudioReceiver::on_frames(const std::function<void(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>&, const std::map<uint32_t, int64_t>&)>& callback) {
        frames_callback_ = callback;
    }

    void AudioReceiver::open() {
        sink_ = std::make_shared<wrtc::interfaces::media::RemoteAudioSink>([this](const std::vector<std::unique_ptr<wrtc::models::AudioFrame>>& samples) {
            if (!description_) {
                return;
            }
            if (!weak_sink_.lock()) {
                return;
            }
            const std::lock_guard lock(mutex_);
            std::map<uint32_t, std::pair<bytes::unique_binary, size_t>> processed_frames;
            std::map<uint32_t, int64_t> timestamps;
            for (const auto& frame : samples) {
                try {
                    bytes::unique_binary data = bytes::make_unique_binary(frame->size);
                    std::memcpy(data.get(), frame->data, frame->size);
                    processed_frames.emplace(
                        frame->ssrc,
                        std::pair{
                            resample_frame(
                                frame->ssrc,
                                std::move(data),
                                frame->size,
                                frame->channels,
                                frame->sample_rate
                            ),
                            frame_size()
                        }
                    );
                    timestamps[frame->ssrc] = frame->absolute_capture_timestamp_ms;
                } catch (const InvalidParams& e) {
                    RTC_LOG(LS_ERROR) << "Failed to adapt audio frame: " << e.what();
                }
            }
            frames_++;
            (void) frames_callback_(processed_frames, timestamps);
        });
        weak_sink_ = sink_;
    }

    std::weak_ptr<wrtc::interfaces::media::RemoteAudioSink> AudioReceiver::remote_sink() {
        return sink_;
    }
} // ntgcalls::media
