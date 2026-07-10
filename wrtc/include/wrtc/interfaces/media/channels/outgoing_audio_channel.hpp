//
// Created by Lauren on 31/03/24.
//

#pragma once
#include <call/call.h>
#include <pc/dtls_srtp_transport.h>
#include <pc/rtp_sender.h>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/media/frame_transformer.hpp>
#include <wrtc/models/media_content.hpp>

namespace wrtc::interfaces::media::channels {

    class OutgoingAudioChannel {
        uint32_t ssrc_ = 0;
        std::unique_ptr<webrtc::BaseChannel> channel_;
        utils::SafeThread& worker_thread_;
        utils::SafeThread& network_thread_;
        webrtc::LocalAudioSinkAdapter* sink_;

    public:
        OutgoingAudioChannel(
            webrtc::Call* call,
            ChannelManager* channel_manager,
            webrtc::RtpTransport* rtp_transport,
            const models::MediaContent& media_content,
            utils::SafeThread& worker_thread,
            utils::SafeThread& network_thread,
            webrtc::LocalAudioSinkAdapter* sink,
            const std::map<int32_t, FrameTransformer::PayloadType>& payload_type_mapping,
            E2EEncryptor* encryptor,
            const std::function<std::pair<uint8_t, bool>()>& get_audio_level_and_speech
        );

        void set_enabled(bool enable) const;

        ~OutgoingAudioChannel();

        [[nodiscard]] uint32_t ssrc() const;
    };

} // wrtc::interfaces::media::channels