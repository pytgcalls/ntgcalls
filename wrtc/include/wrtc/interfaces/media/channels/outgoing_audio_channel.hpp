//
// Created by Laky64 on 31/03/2024.
//

#pragma once
#include <call/call.h>
#include <pc/dtls_srtp_transport.h>
#include <pc/rtp_sender.h>

#include <wrtc/models/media_content.hpp>
#include <wrtc/interfaces/media/channel_manager.hpp>

namespace wrtc {

    class OutgoingAudioChannel {
        uint32_t _ssrc = 0;
        std::unique_ptr<webrtc::VoiceChannel> channel;
        SafeThread& workerThread;
        SafeThread& networkThread;
        webrtc::LocalAudioSinkAdapter* sink;

    public:
        OutgoingAudioChannel(
            webrtc::Call* call,
            ChannelManager* channelManager,
            webrtc::RtpTransport* rtpTransport,
            const MediaContent& mediaContent,
            SafeThread& workerThread,
            SafeThread& networkThread,
            webrtc::LocalAudioSinkAdapter* sink
        );

        void set_enabled(bool enable) const;

        ~OutgoingAudioChannel();

        [[nodiscard]] uint32_t ssrc() const;
    };

} // wrtc