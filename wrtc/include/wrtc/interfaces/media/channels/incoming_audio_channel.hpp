//
// Created by Lauren on 03/10/24.
//

#pragma once
#include <call/call.h>
#include <pc/dtls_srtp_transport.h>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/media/frame_transformer.hpp>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>
#include <wrtc/models/media_content.hpp>

namespace wrtc::interfaces::media::channels {

    class IncomingAudioChannel {
        uint32_t ssrc_ = 0;
        std::unique_ptr<webrtc::BaseChannel> channel_;
        utils::SafeThread& worker_thread_;
        utils::SafeThread& network_thread_;
        int64_t activity_timestamp_ = 0;

    public:
        IncomingAudioChannel(
            webrtc::Call* call,
            ChannelManager* channel_manager,
            webrtc::RtpTransport* rtp_transport,
            const models::MediaContent& media_content,
            utils::SafeThread& worker_thread,
            utils::SafeThread& network_thread,
            std::weak_ptr<RemoteAudioSink> remote_audio_sink,
            const std::map<int32_t, FrameTransformer::PayloadType>& payload_type_mapping,
            E2EEncryptor* encryptor,
            const std::function<void(uint32_t, uint8_t, bool)>& set_audio_level_and_speech
        );

        ~IncomingAudioChannel();

        void update_activity();

        [[nodiscard]] int64_t get_activity() const;

        [[nodiscard]] uint32_t ssrc() const;
    };

} // wrtc::interfaces::media::channels
