//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <wrtc/utils/encoded/encoded_queue.hpp>
#include <api/call/audio_sink.h>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>

namespace wrtc::interfaces::media {

    class RawAudioSink final: public webrtc::AudioSinkInterface {
        std::function<void(std::unique_ptr<models::AudioFrame>)> callback_;
        std::shared_ptr<utils::encoded::EncodedQueue> encoded_;
        uint32_t ssrc_ = 0;

    public:
        ~RawAudioSink() override;

        void OnData(const Data& audio) override;

        void set_remote_audio_sink(uint32_t ssrc, std::function<void(std::unique_ptr<models::AudioFrame>)> callback);

        void set_encoded_queue(std::shared_ptr<utils::encoded::EncodedQueue> queue);
    };

} // wrtc::interfaces::media
