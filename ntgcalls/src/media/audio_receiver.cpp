//
// Created by Laky64 on 07/10/24.
//

#include <ntgcalls/media/audio_receiver.hpp>
#include <ntgcalls/exceptions.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls {
    AudioReceiver::AudioReceiver() {
        resampler = std::make_unique<webrtc::Resampler>();
        sink = std::make_unique<wrtc::RemoteAudioSink>([this](const std::vector<std::shared_ptr<wrtc::AudioFrame>>& frames) {
            if (!description) {
                return;
            }
            std::map<uint32_t, bytes::shared_binary> processedFrames;
            for (const auto& frame: frames) {
                try {
                    processedFrames[frame->ssrc] = resampleFrame(
                        std::move(frame->data),
                        frame->size,
                        frame->channels,
                        frame->sampleRate
                    );
                } catch (const InvalidParams& e) {
                    RTC_LOG(LS_ERROR) << "Failed to adapt audio frame: " << e.what();
                }
            }
            (void) framesCallback(processedFrames);
        });
    }

    bytes::shared_binary AudioReceiver::resampleFrame(bytes::unique_binary data, const size_t size, const uint8_t channels, const uint16_t sampleRate) {
        bytes::unique_binary convertedData;
        size_t newSize;
        if (channels != description->channelCount) {
            switch (channels){
            case 1:
                convertedData = monoToStereo(data, size, &newSize);
                break;
            case 2:
                convertedData = stereoToMono(data, size, &newSize);
                break;
            default:
                RTC_LOG(LS_ERROR) << "Unsupported audio channels count: " << std::to_string(channels);
                throw InvalidParams("Unsupported audio channels count: " + std::to_string(channels));
            }
        } else {
            newSize = size;
            convertedData = std::move(data);
        }
        const auto newFrame = bytes::make_shared_binary(frameSize());
        if (description->sampleRate == sampleRate) {
            memcpy(newFrame.get(), convertedData.get(), newSize);
        } else {
            resampler->ResetIfNeeded(sampleRate, description->sampleRate, channels);
            size_t newFrameSize = 0;
            const auto resampled = resampler->Push(
                reinterpret_cast<const int16_t*>(convertedData.get()),
                newSize,
                reinterpret_cast<int16_t*>(newFrame.get()),
                frameSize(),
                newFrameSize
            );
            if (resampled != 0) {
                RTC_LOG(LS_ERROR) << "Failed to resample audio frame";
                throw InvalidParams("Failed to resample audio frame");
            }
        }
        return newFrame;
    }

    bytes::unique_binary AudioReceiver::stereoToMono(const bytes::unique_binary& data, const size_t size, size_t *newSize) {
        *newSize = size / 2;
        auto monoData = bytes::make_unique_binary(*newSize);
        for (size_t i = 0; i < size; i += 4) {
            const auto left = reinterpret_cast<int16_t*>(data.get())[i];
            const auto right = reinterpret_cast<int16_t*>(data.get())[i + 2];
            reinterpret_cast<int16_t*>(monoData.get())[i / 2] = (left + right) / 2;
        }
        return std::move(monoData);
    }

    bytes::unique_binary AudioReceiver::monoToStereo(const bytes::unique_binary& data, const size_t size, size_t *newSize) {
        *newSize = size * 2;
        auto stereoData = bytes::make_unique_binary(*newSize);
        for (size_t i = 0; i < size; i += 2) {
            const auto sample = reinterpret_cast<int16_t*>(data.get())[i];
            reinterpret_cast<int16_t*>(stereoData.get())[i * 2] = sample;
            reinterpret_cast<int16_t*>(stereoData.get())[i * 2 + 2] = sample;
        }
        return std::move(stereoData);
    }

    void AudioReceiver::onFrames(const std::function<void(std::map<uint32_t, bytes::shared_binary>)>& callback) {
        framesCallback = callback;
    }

    wrtc::RemoteMediaInterface* AudioReceiver::remoteSink() {
        return sink.get();
    }
} // ntgcalls