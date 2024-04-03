//
// Created by Laky64 on 02/04/2024.
//

#pragma once
#include <call/call.h>
#include <pc/dtls_srtp_transport.h>

#include "wrtc/interfaces/media/media_content.hpp"
#include "wrtc/interfaces/media/channel_manager.hpp"

namespace wrtc {
    class OutgoingVideoChannel : public sigslot::has_slots<> {
        uint32_t _ssrc = 0;
        std::unique_ptr<cricket::VideoChannel> channel;
        rtc::Thread* workerThread;
        rtc::Thread* networkThread;
        std::unique_ptr<webrtc::VideoBitrateAllocatorFactory> bitrateAllocatorFactory;

    public:
        OutgoingVideoChannel(
            webrtc::Call* call,
            ChannelManager* channelManager,
            webrtc::RtpTransport* rtpTransport,
            const MediaContent& mediaContent,
            rtc::Thread* workerThread,
            rtc::Thread* networkThread,
            webrtc::VideoTrackSourceInterface* sink
        );

        ~OutgoingVideoChannel() override;

        [[nodiscard]] uint32_t ssrc() const;
    };
} // wrtc
