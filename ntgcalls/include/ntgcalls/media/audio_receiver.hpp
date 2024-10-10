//
// Created by Laky64 on 07/10/24.
//

#pragma once
#include <map>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>
#include <ntgcalls/media/base_receiver.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <common_audio/resampler/include/resampler.h>

namespace ntgcalls {

    class AudioReceiver final: public AudioSink, public BaseReceiver, public std::enable_shared_from_this<AudioReceiver> {
        wrtc::synchronized_callback<const std::map<uint32_t, bytes::unique_binary>&> framesCallback;
        std::unique_ptr<wrtc::RemoteAudioSink> sink;
        std::unique_ptr<webrtc::Resampler> resampler;

        bytes::unique_binary resampleFrame(bytes::unique_binary data, size_t size, uint8_t channels, uint16_t sampleRate);

        static bytes::unique_binary stereoToMono(const bytes::unique_binary& data, size_t size, size_t *newSize);

        static bytes::unique_binary monoToStereo(const bytes::unique_binary& data, size_t size, size_t *newSize);

    public:
        AudioReceiver();

        ~AudioReceiver() override;

        wrtc::RemoteMediaInterface* remoteSink() override;

        void onFrames(const std::function<void(const std::map<uint32_t, bytes::unique_binary>&)>& callback);

        void open() override;
    };

} // ntgcalls
