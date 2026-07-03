//
// Created by Lauren on 01/04/24.
//

#pragma once
#include <media/base/media_engine.h>
#include <pc/channel.h>
#include <wrtc/utils/safe_thread.hpp>

namespace wrtc::interfaces::media {
    class ChannelManager {
        const webrtc::Environment& environment_;
        webrtc::MediaEngineInterface* media_engine_;
        utils::SafeThread& worker_thread_;
        webrtc::Thread* signaling_thread_;
        webrtc::Thread* network_thread_;
        webrtc::UniqueRandomIdGenerator ssrc_generator_;

    public:
        ChannelManager(
            const webrtc::Environment& environment,
            webrtc::MediaEngineInterface* media_engine,
            utils::SafeThread& worker_thread,
            webrtc::Thread* network_thread,
            webrtc::Thread* signaling_thread
        );

        std::unique_ptr<webrtc::BaseChannel> create_voice_channel(
            webrtc::Call* call,
            const webrtc::MediaConfig& media_config,
            const std::string& mid,
            bool srtp_required,
            const webrtc::CryptoOptions& crypto_options,
            const webrtc::AudioOptions& options
        );

        std::unique_ptr<webrtc::BaseChannel> create_video_channel(
            webrtc::Call* call,
            const webrtc::MediaConfig& media_config,
            const std::string& mid,
            bool srtp_required,
            const webrtc::CryptoOptions& crypto_options,
            const webrtc::VideoOptions& options,
            webrtc::VideoBitrateAllocatorFactory* bitrate_allocator_factory
        );
    };
} // wrtc::interfaces::media