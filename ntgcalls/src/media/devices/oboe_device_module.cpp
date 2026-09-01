//
// Created by Lauren on 24/04/25.
//

#ifdef IS_ANDROID
#include <thread>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/devices/oboe_device_module.hpp>

namespace ntgcalls::media::devices {

    OboeDeviceModule::OboeDeviceModule(const AudioDescription* desc, const bool is_capture, BaseSink* sink):
    BaseIO(sink),
    BaseDeviceModule(desc, is_capture),
    BaseReader(sink),
    AudioMixer(sink) {
        frame_size_ = static_cast<size_t>(sink->frame_size());
        if (const auto r = create_stream(); r != oboe::Result::OK) {
            throw MediaDeviceError("Failed to open Oboe stream: " + std::string(oboe::convertToText(r)));
        }
    }

    OboeDeviceModule::~OboeDeviceModule() {
        if (stream_) {
            stream_->close();
        }
    }

    void OboeDeviceModule::on_data(const bytes::unique_binary data) {
        const std::lock_guard lock(buffer_mutex_);
        const auto* src = reinterpret_cast<const bytes::byte*>(data.get());
        buffer_.insert(buffer_.end(), src, src + frame_size_);
    }

    oboe::DataCallbackResult OboeDeviceModule::onAudioReady(oboe::AudioStream* audio_stream, void* audio_data, const int32_t num_frames) {
        const size_t bytes_per_frame = audio_stream->getBytesPerFrame();
        const size_t required_bytes = num_frames * bytes_per_frame;
        if (is_capture_) {
            const auto* src = static_cast<const bytes::byte*>(audio_data);
            buffer_.insert(buffer_.end(), src, src + required_bytes);
            while (buffer_.size() >= frame_size_) {
                auto result = bytes::make_unique_binary(frame_size_);
                std::memcpy(result.get(), buffer_.data(), frame_size_);
                buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(frame_size_));
                data_callback_(std::move(result), {});
            }
        } else {
            const std::lock_guard lock(buffer_mutex_);
            auto* dest = static_cast<bytes::byte*>(audio_data);
            const size_t available = buffer_.size();
            const size_t bytes_to_copy = std::min(available, required_bytes);
            if (bytes_to_copy > 0) {
                std::memcpy(dest, buffer_.data(), bytes_to_copy);
                buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(bytes_to_copy));
            }
            if (bytes_to_copy < required_bytes) {
                std::memset(dest + bytes_to_copy, 0, required_bytes - bytes_to_copy);
            }
        }
        return oboe::DataCallbackResult::Continue;
    }

    oboe::Result OboeDeviceModule::create_stream() {
        RTC_LOG(LS_INFO) << "OboeDeviceModule creating stream";
        oboe::AudioStreamBuilder builder;
        builder.setDirection(is_capture_ ? oboe::Direction::Input : oboe::Direction::Output)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSampleRate(static_cast<int32_t>(rate_))
            ->setChannelCount(channels_)
            ->setFormat(oboe::AudioFormat::I16)
            ->setUsage(oboe::Usage::VoiceCommunication)
            ->setContentType(oboe::ContentType::Speech)
            ->setCallback(this);

        if (is_capture_) {
            builder.setInputPreset(oboe::InputPreset::VoiceCommunication);
        }

        const auto r = builder.openStream(stream_);
        if (r == oboe::Result::OK) {
            stream_->setBufferSizeInFrames(stream_->getFramesPerBurst() * 2);
            RTC_LOG(LS_INFO) << "OboeDeviceModule stream created successfully";
        }
        return r;
    }

    void OboeDeviceModule::restart_stream() {
        if (restart_required_.exchange(true)) {
            return;
        }

        RTC_LOG(LS_INFO) << "OboeDeviceModule restarting stream";
        if (stream_) {
            stream_->close();
            stream_ = nullptr;
        }

        {
            const std::lock_guard lock(buffer_mutex_);
            buffer_.clear();
        }

        if (create_stream() == oboe::Result::OK) {
            stream_->requestStart();
        }

        restart_required_ = false;
    }

    void OboeDeviceModule::onErrorAfterClose(oboe::AudioStream* audio_stream, const oboe::Result error) {
        RTC_LOG(LS_WARNING) << "OboeDeviceModule stream error: " << oboe::convertToText(error);
        if (error == oboe::Result::ErrorDisconnected) {
            restart_stream();
        }
    }

    void OboeDeviceModule::open() {
        if (stream_) {
            if (const auto r = stream_->requestStart(); r != oboe::Result::OK) {
                throw MediaDeviceError("Failed to start Oboe stream: " + std::string(oboe::convertToText(r)));
            }
        } else {
            throw MediaDeviceError("Stream is not initialized");
        }
    }

} // ntgcalls::media::devices

#endif
