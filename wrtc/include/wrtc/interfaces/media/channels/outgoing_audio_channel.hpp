//
// Created by Lauren on 31/03/24.
//

#pragma once
#include <wrtc/audio_factory/audio_encoder_factory.hpp>
#include <call/call.h>
#include <pc/dtls_srtp_transport.h>
#include <pc/rtp_sender.h>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/media/frame_transformer.hpp>
#include <wrtc/models/audio_encoding.hpp>
#include <wrtc/models/media_content.hpp>

namespace wrtc::interfaces::media::channels {

    class OutgoingAudioChannel {
        uint32_t ssrc_ = 0;
        std::unique_ptr<webrtc::BaseChannel> channel_;
        utils::SafeThread& network_thread_;
        webrtc::LocalAudioSinkAdapter* sink_;
        models::AudioEncoding encoding_;
        audio_factory::AudioEncoderFactory* encoder_factory_;
        std::optional<uint64_t> queue_id_;

        static constexpr uint32_t kMaxPayloadSize = 1000;

        static int packet_time(const std::optional<uint32_t>& bitrate);

    public:
        OutgoingAudioChannel(
            webrtc::Call* call,
            ChannelManager* channel_manager,
            audio_factory::AudioEncoderFactory* encoder_factory,
            webrtc::RtpTransport* rtp_transport,
            const models::MediaContent& media_content,
            utils::SafeThread& network_thread,
            webrtc::LocalAudioSinkAdapter* sink,
            const std::map<int32_t, FrameTransformer::PayloadType>& payload_type_mapping,
            E2EEncryptor* encryptor,
            const std::function<std::pair<uint8_t, bool>()>& get_audio_level_and_speech,
            const models::AudioEncoding& encoding
        );

        void set_enabled(bool enable) const;

        ~OutgoingAudioChannel();

        [[nodiscard]] uint32_t ssrc() const;

        [[nodiscard]] models::AudioEncoding encoding() const;
    };

} // wrtc::interfaces::media::channels
