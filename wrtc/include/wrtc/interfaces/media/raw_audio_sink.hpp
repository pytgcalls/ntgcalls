//
// Created by Laky64 on 07/10/24.
//

#pragma once
#include <api/call/audio_sink.h>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>

namespace wrtc {

    class RawAudioSink final : public webrtc::AudioSinkInterface  {
        std::function<void(std::unique_ptr<AudioFrame>)> callbackData;
        uint32_t ssrc = 0;

    public:
        void OnData(const Data& audio) override;

        void setRemoteAudioSink(uint32_t ssrc, std::function<void(std::unique_ptr<AudioFrame>)> callback);
    };

} // wrtc
