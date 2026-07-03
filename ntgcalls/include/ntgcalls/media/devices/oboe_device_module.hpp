//
// Created by Lauren on 24/04/25.
//

#pragma once

#ifdef IS_ANDROID
#include <ntgcalls/io/audio_mixer.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/devices/base_device_module.hpp>
#include <oboe/Oboe.h>

namespace ntgcalls::media::devices {

    class OboeDeviceModule final: public BaseDeviceModule, public io::BaseReader, public io::AudioMixer, oboe::AudioStreamCallback {
        std::shared_ptr<oboe::AudioStream> stream_;
        std::mutex buffer_mutex_;
        std::vector<uint8_t> buffer_;
        size_t frame_size_ = 0;
        std::atomic_bool restart_required_;

        oboe::Result create_stream();

        void restart_stream();

    protected:
        void on_data(bytes::unique_binary data) override;

    public:
        OboeDeviceModule(const AudioDescription* desc, bool is_capture, BaseSink* sink);

        ~OboeDeviceModule() override;

        void open() override;

        oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audio_stream, void* audio_data, int32_t num_frames) override;

        void onErrorAfterClose(oboe::AudioStream* audio_stream, oboe::Result error) override;
    };

} // ntgcalls::media::devices
#endif