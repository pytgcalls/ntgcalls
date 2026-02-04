//
// Created by Laky64 on 24/04/25.
//

#pragma once

#ifdef IS_ANDROID
#include <jni.h>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/base_device_module.hpp>
#include <ntgcalls/io/audio_mixer.hpp>
#include <oboe/Oboe.h>

namespace ntgcalls {

    class OboeDeviceModule final: public BaseDeviceModule, public BaseReader, public AudioMixer, oboe::AudioStreamCallback {
        std::shared_ptr<oboe::AudioStream> stream;
        std::mutex bufferMutex;
        std::vector<uint8_t> buffer;
        size_t frameSize_ = 0;
        std::atomic_bool restartRequired_;

        void onData(bytes::unique_binary data) override;

        oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audioStream, void* audioData, int32_t numFrames) override;

        void onErrorAfterClose(oboe::AudioStream* audioStream, oboe::Result error) override;

        oboe::Result createStream();

        void restartStream();

    public:
        OboeDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink* sink);

        ~OboeDeviceModule() override;

        void open() override;
    };

} // ntgcalls
#endif