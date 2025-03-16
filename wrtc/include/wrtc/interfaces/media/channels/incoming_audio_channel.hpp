//
// Created by Laky64 on 03/10/24.
//

#pragma once
#include <call/call.h>
#include <pc/dtls_srtp_transport.h>

#include <wrtc/models/media_content.hpp>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>

namespace wrtc {

    class IncomingAudioChannel final : public sigslot::has_slots<> {
        uint32_t _ssrc = 0;
        std::unique_ptr<cricket::VoiceChannel> channel;
        rtc::Thread* workerThread;
        rtc::Thread* networkThread;
        int64_t activityTimestamp = 0;

    public:
        IncomingAudioChannel(
            webrtc::Call* call,
            ChannelManager *channelManager,
            webrtc::RtpTransport* rtpTransport,
            const MediaContent& mediaContent,
            rtc::Thread *workerThread,
            rtc::Thread* networkThread,
            std::weak_ptr<RemoteAudioSink> remoteAudioSink
        );

        ~IncomingAudioChannel() override;

        void updateActivity();

        [[nodiscard]] int64_t getActivity() const;

        uint32_t ssrc() const;
    };

} // wrtc
