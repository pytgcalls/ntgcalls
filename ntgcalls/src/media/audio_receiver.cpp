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
                size_t convFrameSize;
                bytes::unique_binary convertedFrame;
                try {
                    convertedFrame = adaptFrame(
                        std::move(frame->data),
                        frame->size,
                        frame->channels,
                        &convFrameSize
                    );
                } catch (const InvalidParams& e) {
                    RTC_LOG(LS_ERROR) << "Failed to adapt audio frame: " << e.what();
                    continue;
                }

                resampler->ResetIfNeeded(frame->sampleRate, description->sampleRate, frame->channels);

                size_t newFrameSize = 0;
                const auto newFrame = bytes::make_shared_binary(frameSize());
                const auto resampled = resampler->Push(
                    reinterpret_cast<const int16_t*>(convertedFrame.get()),
                    convFrameSize,
                    reinterpret_cast<int16_t*>(newFrame.get()),
                    frameSize(),
                    newFrameSize
                );
                if (resampled != 0) {
                    RTC_LOG(LS_ERROR) << "Failed to resample audio frame";
                    continue;
                }

                processedFrames[frame->ssrc] = newFrame;
            }
            (void) framesCallback(processedFrames);
        });
    }

    bytes::unique_binary AudioReceiver::adaptFrame(bytes::unique_binary data, const size_t size, const uint8_t channels, size_t *newSize) const {
        if (channels != description->channelCount) {
            switch (channels){
            case 1:
                return monoToStereo(data, size, newSize);
            case 2:
                return stereoToMono(data, size, newSize);
            default:
                RTC_LOG(LS_ERROR) << "Unsupported audio channels count: " << std::to_string(channels);
                throw InvalidParams("Unsupported audio channels count: " + std::to_string(channels));
            }
        }
        return std::move(data);
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