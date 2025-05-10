//
// Created by Laky64 on 24/04/25.
//

#ifdef IS_ANDROID
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/devices/oboe_device_module.hpp>

namespace ntgcalls {

    OboeDeviceModule::OboeDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink* sink):
        BaseIO(sink),
        BaseDeviceModule(desc, isCapture),
        BaseReader(sink),
        AudioMixer(sink)
    {
        RTC_LOG(LS_ERROR) << "OboeDeviceModule initializing";
        oboe::AudioStreamBuilder builder;
        const auto r = builder.setDirection(isCapture ? oboe::Direction::Input : oboe::Direction::Output)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSampleRate(static_cast<int32_t>(desc->sampleRate))
            ->setChannelCount(desc->channelCount)
            ->setCallback(this)
            ->openStream(stream);

        RTC_LOG(LS_ERROR) << "OboeDeviceModule initialized";
        if (r != oboe::Result::OK) {
            throw MediaDeviceError("Failed to open Oboe stream: " + std::string(oboe::convertToText(r)));
        }
        RTC_LOG(LS_ERROR) << "OboeDeviceModule opened";


        stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);
    }

    OboeDeviceModule::~OboeDeviceModule() {
        if (stream) {
            stream->close();
        }
    }

    void OboeDeviceModule::onData(bytes::unique_binary data) {
        std::lock_guard lock(queueMutex);
        queue.push(std::move(data));
    }

    oboe::DataCallbackResult OboeDeviceModule::onAudioReady(oboe::AudioStream* audioStream, void* audioData, int32_t numFrames) {
        const auto frameSize = sink->frameSize();
        if (isCapture) {
            auto result = bytes::make_unique_binary(frameSize * numFrames);
            memcpy(result.get(), audioData, frameSize * numFrames);
            dataCallback(std::move(result), {});
        } else {
            std::lock_guard lock(queueMutex);
            if (!queue.empty()) {
                memcpy(audioData, queue.front().get(), frameSize * numFrames);
                queue.pop();
            }
        }
        return oboe::DataCallbackResult::Continue;
    }

    bool OboeDeviceModule::isSupported() {
        return true;
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