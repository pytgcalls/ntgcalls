//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <map>
#include <common_audio/resampler/include/resampler.h>
#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/base_receiver.hpp>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace ntgcalls::media {

    class AudioReceiver final: public AudioSink, public BaseReceiver {
        wrtc::utils::synchronized_callback<void(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>&)> frames_callback_;
        std::shared_ptr<wrtc::interfaces::media::RemoteAudioSink> sink_;
        std::unique_ptr<webrtc::Resampler> resampler_;

        bytes::unique_binary resample_frame(bytes::unique_binary data, size_t size, uint8_t channels, uint16_t sample_rate);

        static bytes::unique_binary stereo_to_mono(const bytes::unique_binary& data, size_t size, size_t *new_size);

        static bytes::unique_binary mono_to_stereo(const bytes::unique_binary& data, size_t size, size_t *new_size);

    public:
        AudioReceiver();

        ~AudioReceiver() override;

        std::weak_ptr<wrtc::interfaces::media::RemoteAudioSink> remote_sink();

        void on_frames(const std::function<void(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>&)>& callback);

        void open() override;
    };

} // ntgcalls::media
