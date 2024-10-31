//
// Created by Laky64 on 01/04/2024.
//

#pragma once
#include <pc/channel.h>
#include <media/base/media_engine.h>

namespace wrtc {
    class ChannelManager {
        cricket::MediaEngineInterface* mediaEngine;
        rtc::Thread* signalingThread;
        rtc::Thread* workerThread;
        rtc::Thread* networkThread;
        rtc::UniqueRandomIdGenerator ssrcGenerator;

    public:
        ChannelManager(
            cricket::MediaEngineInterface* mediaEngine,
            rtc::Thread* workerThread,
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread
        );

        std::unique_ptr<cricket::VoiceChannel> CreateVoiceChannel(
            webrtc::Call* call,
            const cricket::MediaConfig& mediaConfig,
            const std::string& mid,
            bool srtpRequired,
            const webrtc::CryptoOptions& cryptoOptions,
            const cricket::AudioOptions& options
        );

        std::unique_ptr<cricket::VideoChannel>  CreateVideoChannel(
            webrtc::Call* call,
            const cricket::MediaConfig& mediaConfig,
            const std::string& mid,
            bool srtpRequired,
            const webrtc::CryptoOptions& cryptoOptions,
            const cricket::VideoOptions& options,
            webrtc::VideoBitrateAllocatorFactory* bitrateAllocatorFactory
        );

        void DestroyChannel(cricket::ChannelInterface* channel);
    };
} // wrtc