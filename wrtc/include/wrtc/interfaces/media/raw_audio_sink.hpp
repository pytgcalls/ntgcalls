//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <wrtc/audio_factory/opus/opus_jitter_buffer.hpp>
#include <wrtc/utils/encoded/encoded_queue.hpp>
#include <api/call/audio_sink.h>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>

namespace wrtc::interfaces::media {

    class RawAudioSink final: public webrtc::AudioSinkInterface {
        std::weak_ptr<RemoteAudioSink> remote_sink_;
        std::shared_ptr<utils::encoded::EncodedQueue> encoded_;
        audio_factory::OpusJitterBuffer jitter_buffer_;
        uint32_t ssrc_ = 0;

        void send_encoded(const std::shared_ptr<RemoteAudioSink>& remote_sink, const Data& audio, const utils::encoded::EncodedFrame* packet) const;

    public:
        void OnData(const Data& audio) override;

        void set_remote_audio_sink(uint32_t ssrc, std::weak_ptr<RemoteAudioSink> remote_sink);

        void set_encoded_queue(std::shared_ptr<utils::encoded::EncodedQueue> queue);
    };

} // wrtc::interfaces::media
