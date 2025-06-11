//
// Created by Laky64 on 01/04/2024.
//

#pragma once
#include <pc/channel.h>
#include <media/base/media_engine.h>

namespace wrtc {
    class ChannelManager {
        webrtc::MediaEngineInterface* mediaEngine;
        webrtc::Thread* signalingThread;
        webrtc::Thread* workerThread;
        webrtc::Thread* networkThread;
        webrtc::UniqueRandomIdGenerator ssrcGenerator;

    public:
        ChannelManager(
            webrtc::MediaEngineInterface* mediaEngine,
            webrtc::Thread* workerThread,
            webrtc::Thread* networkThread,
            webrtc::Thread* signalingThread
        );

        std::unique_ptr<webrtc::VoiceChannel> CreateVoiceChannel(
            webrtc::Call* call,
            const webrtc::MediaConfig& mediaConfig,
            const std::string& mid,
            bool srtpRequired,
            const webrtc::CryptoOptions& cryptoOptions,
            const webrtc::AudioOptions& options
        );

        std::unique_ptr<webrtc::VideoChannel>  CreateVideoChannel(
            webrtc::Call* call,
            const webrtc::MediaConfig& mediaConfig,
            const std::string& mid,
            bool srtpRequired,
            const webrtc::CryptoOptions& cryptoOptions,
            const webrtc::VideoOptions& options,
            webrtc::VideoBitrateAllocatorFactory* bitrateAllocatorFactory
        );
    };
} // wrtc