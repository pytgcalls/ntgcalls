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

    class IncomingAudioChannel {
        uint32_t _ssrc = 0;
        std::unique_ptr<webrtc::VoiceChannel> channel;
        SafeThread& workerThread;
        SafeThread& networkThread;
        int64_t activityTimestamp = 0;

    public:
        IncomingAudioChannel(
            webrtc::Call* call,
            ChannelManager* channelManager,
            webrtc::RtpTransport* rtpTransport,
            const MediaContent& mediaContent,
            SafeThread& workerThread,
            SafeThread& networkThread,
            std::weak_ptr<RemoteAudioSink> remoteAudioSink
        );

        ~IncomingAudioChannel();

        void updateActivity();

        [[nodiscard]] int64_t getActivity() const;

        [[nodiscard]] uint32_t ssrc() const;
    };

} // wrtc
