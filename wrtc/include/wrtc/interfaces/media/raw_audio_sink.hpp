//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <api/call/audio_sink.h>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>

namespace wrtc::interfaces::media {

    class RawAudioSink final: public webrtc::AudioSinkInterface {
        std::function<void(std::unique_ptr<models::AudioFrame>)> callback_;
        uint32_t ssrc_ = 0;

    public:
        ~RawAudioSink() override;

        void OnData(const Data& audio) override;

        void set_remote_audio_sink(uint32_t ssrc, std::function<void(std::unique_ptr<models::AudioFrame>)> callback);
    };

} // wrtc::interfaces::media
