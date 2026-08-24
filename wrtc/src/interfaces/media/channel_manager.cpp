//
// Created by Lauren on 01/04/24.
//

#include <wrtc/interfaces/media/channel_manager.hpp>

#include <rtc_base/trace_event.h>

namespace wrtc::interfaces::media {
    ChannelManager::ChannelManager(
        const webrtc::Environment& environment,
        webrtc::MediaEngineInterface* media_engine,
        utils::SafeThread& worker_thread,
        webrtc::Thread* network_thread,
        webrtc::Thread* signaling_thread
    ): environment_(environment), media_engine_(media_engine), worker_thread_(worker_thread), signaling_thread_(signaling_thread), network_thread_(network_thread) {
        RTC_DCHECK_RUN_ON(signaling_thread);
        RTC_DCHECK(&worker_thread);
        RTC_DCHECK(network_thread);
    }

    std::unique_ptr<webrtc::BaseChannel> ChannelManager::create_voice_channel(
        webrtc::Call* call,
        const webrtc::MediaConfig& media_config,
        const std::string& mid,
        const bool srtp_required,
        const webrtc::CryptoOptions& crypto_options,
        const webrtc::AudioOptions& options
    ) {
        RTC_DCHECK(call);
        RTC_DCHECK(media_engine_);
        if (!worker_thread_.IsCurrent()) {
            std::unique_ptr<webrtc::BaseChannel> temp;
            worker_thread_.BlockingCall([&] {
                temp = create_voice_channel(call, media_config, mid, srtp_required, crypto_options, options);
            });
            return std::move(temp);
        }
        RTC_DCHECK_RUN_ON(&worker_thread_);
        auto send_media_channel = media_engine_->voice().CreateSendChannel(
            environment_,
            call,
            media_config,
            options,
            crypto_options
        );
        if (!send_media_channel) {
            return nullptr;
        }
        auto receive_media_channel = media_engine_->voice().CreateReceiveChannel(
            environment_,
            call,
            media_config,
            options,
            crypto_options
        );
        if (!receive_media_channel) {
            return nullptr;
        }
        return std::make_unique<webrtc::BaseChannel>(
            worker_thread_,
            network_thread_,
            signaling_thread_,
            std::move(send_media_channel),
            std::move(receive_media_channel),
            mid,
            webrtc::MediaType::AUDIO,
            srtp_required,
            crypto_options,
            &ssrc_generator_
        );
    }

    std::unique_ptr<webrtc::BaseChannel> ChannelManager::create_video_channel(
        webrtc::Call* call,
        const webrtc::MediaConfig& media_config,
        const std::string& mid,
        const bool srtp_required,
        const webrtc::CryptoOptions& crypto_options,
        const webrtc::VideoOptions& options,
        webrtc::VideoBitrateAllocatorFactory* bitrate_allocator_factory
    ) {
        RTC_DCHECK(call);
        RTC_DCHECK(media_engine_);
        if (!worker_thread_.IsCurrent()) {
            std::unique_ptr<webrtc::BaseChannel> temp = nullptr;
            worker_thread_.BlockingCall([&] {
                temp = create_video_channel(
                    call,
                    media_config,
                    mid,
                    srtp_required,
                    crypto_options,
                    options,
                    bitrate_allocator_factory
                );
            });
            return temp;
        }
        RTC_DCHECK_RUN_ON(&worker_thread_);
        std::unique_ptr<webrtc::VideoMediaSendChannelInterface> send_media_channel = media_engine_->video().CreateSendChannel(
            environment_,
            call,
            media_config,
            options,
            crypto_options,
            bitrate_allocator_factory,
            nullptr,
            nullptr
        );
        if (!send_media_channel) {
            return nullptr;
        }
        std::unique_ptr<webrtc::VideoMediaReceiveChannelInterface> receive_media_channel = media_engine_->video().CreateReceiveChannel(
            environment_,
            call,
            media_config,
            options,
            crypto_options
        );
        return std::make_unique<webrtc::BaseChannel>(
            worker_thread_,
            network_thread_,
            signaling_thread_,
            std::move(send_media_channel),
            std::move(receive_media_channel),
            mid,
            webrtc::MediaType::VIDEO,
            srtp_required,
            crypto_options,
            &ssrc_generator_
        );
    }

} // wrtc::interfaces::media
