//
// Created by Laky64 on 02/04/2024.
//

#pragma once
#include <call/call.h>
#include <pc/dtls_srtp_transport.h>

#include <wrtc/models/media_content.hpp>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/media/local_video_adapter.hpp>

namespace wrtc {
    class OutgoingVideoChannel final : public sigslot::has_slots<> {
        uint32_t _ssrc = 0;
        std::unique_ptr<cricket::VideoChannel> channel;
        rtc::Thread* workerThread;
        rtc::Thread* networkThread;
        std::unique_ptr<webrtc::VideoBitrateAllocatorFactory> bitrateAllocatorFactory;
        LocalVideoAdapter* sink;

    public:
        OutgoingVideoChannel(
            webrtc::Call* call,
            ChannelManager* channelManager,
            webrtc::RtpTransport* rtpTransport,
            const MediaContent& mediaContent,
            rtc::Thread* workerThread,
            rtc::Thread* networkThread,
            LocalVideoAdapter* sink
        );

        ~OutgoingVideoChannel() override;

        void set_enabled(bool enable) const;

        [[nodiscard]] uint32_t ssrc() const;
    };
} // wrtc