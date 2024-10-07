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

    class AudioReceiver final: public AudioSink, public BaseReceiver {
        wrtc::synchronized_callback<std::map<uint32_t, bytes::shared_binary>> framesCallback;
        std::unique_ptr<wrtc::RemoteAudioSink> sink;
        std::unique_ptr<webrtc::Resampler> resampler;

        bytes::unique_binary adaptFrame(bytes::unique_binary data, size_t size, uint8_t channels, size_t *newSize) const;

        static bytes::unique_binary stereoToMono(const bytes::unique_binary& data, size_t size, size_t *newSize);

        static bytes::unique_binary monoToStereo(const bytes::unique_binary& data, size_t size, size_t *newSize);

    public:
        AudioReceiver();

        wrtc::RemoteMediaInterface* remoteSink() override;

        void onFrames(const std::function<void(std::map<uint32_t, bytes::shared_binary>)>& callback);
    };

} // ntgcalls
