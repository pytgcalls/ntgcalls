//
// Created by Laky64 on 24/04/25.
//

#ifdef IS_ANDROID
#include <thread>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/devices/oboe_device_module.hpp>

namespace ntgcalls {

    OboeDeviceModule::OboeDeviceModule(const AudioDescription* desc, const bool isCapture, BaseSink* sink):
        BaseIO(sink),
        BaseDeviceModule(desc, isCapture),
        BaseReader(sink),
        AudioMixer(sink)
    {
        frameSize_ = static_cast<size_t>(sink->frameSize());
        if (const auto r = createStream(); r != oboe::Result::OK) {
            throw MediaDeviceError("Failed to open Oboe stream: " + std::string(oboe::convertToText(r)));
        }
    }

    OboeDeviceModule::~OboeDeviceModule() {
        if (stream) {
            stream->close();
        }
    }

    void OboeDeviceModule::onData(const bytes::unique_binary data) {
        std::lock_guard lock(bufferMutex);
        const auto* src = reinterpret_cast<const uint8_t*>(data.get());
        buffer.insert(buffer.end(), src, src + frameSize_);
    }

    oboe::DataCallbackResult OboeDeviceModule::onAudioReady(oboe::AudioStream* audioStream, void* audioData, const int32_t numFrames) {
        const size_t bytesPerFrame = audioStream->getBytesPerFrame();
        const size_t requiredBytes = numFrames * bytesPerFrame;
        if (isCapture) {
            const auto* src = static_cast<const uint8_t*>(audioData);
            buffer.insert(buffer.end(), src, src + requiredBytes);
            while (buffer.size() >= frameSize_) {
                auto result = bytes::make_unique_binary(frameSize_);
                memcpy(result.get(), buffer.data(), frameSize_);
                buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(frameSize_));
                dataCallback(std::move(result), {});
            }
        } else {
            std::lock_guard lock(bufferMutex);
            auto* dest = static_cast<uint8_t*>(audioData);
            const size_t available = buffer.size();
            const size_t bytesToCopy = std::min(available, requiredBytes);
            if (bytesToCopy > 0) {
                memcpy(dest, buffer.data(), bytesToCopy);
                buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(bytesToCopy));
            }
            if (bytesToCopy < requiredBytes) {
                memset(dest + bytesToCopy, 0, requiredBytes - bytesToCopy);
            }
        }
        return oboe::DataCallbackResult::Continue;
    }

    oboe::Result OboeDeviceModule::createStream() {
        RTC_LOG(LS_INFO) << "OboeDeviceModule creating stream";
        oboe::AudioStreamBuilder builder;
        builder.setDirection(isCapture ? oboe::Direction::Input : oboe::Direction::Output)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSampleRate(static_cast<int32_t>(rate))
            ->setChannelCount(channels)
            ->setFormat(oboe::AudioFormat::I16)
            ->setUsage(oboe::Usage::VoiceCommunication)
            ->setContentType(oboe::ContentType::Speech)
            ->setCallback(this);

        if (isCapture) {
            builder.setInputPreset(oboe::InputPreset::VoiceCommunication);
        }

        const auto r = builder.openStream(stream);
        if (r == oboe::Result::OK) {
            stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);
            RTC_LOG(LS_INFO) << "OboeDeviceModule stream created successfully";
        }
        return r;
    }

    void OboeDeviceModule::restartStream() {
        if (restartRequired_.exchange(true)) {
            return;
        }

        RTC_LOG(LS_INFO) << "OboeDeviceModule restarting stream";
        if (stream) {
            stream->close();
            stream = nullptr;
        }

        {
            std::lock_guard lock(bufferMutex);
            buffer.clear();
        }

        if (createStream() == oboe::Result::OK) {
            stream->requestStart();
        }

        restartRequired_ = false;
    }

    void OboeDeviceModule::onErrorAfterClose(oboe::AudioStream* audioStream, const oboe::Result error) {
        RTC_LOG(LS_WARNING) << "OboeDeviceModule stream error: " << oboe::convertToText(error);
        if (error == oboe::Result::ErrorDisconnected) {
            restartStream();
        }
    }

    void OboeDeviceModule::open() {
        if (stream) {
            if (const auto r = stream->requestStart(); r != oboe::Result::OK) {
                throw MediaDeviceError("Failed to start Oboe stream: " + std::string(oboe::convertToText(r)));
            }
        } else {
            throw MediaDeviceError("Stream is not initialized");
        }
    }

} // ntgcalls

#endif