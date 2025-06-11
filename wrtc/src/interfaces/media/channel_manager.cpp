//
// Created by Laky64 on 01/04/2024.
//

#include <wrtc/interfaces/media/channel_manager.hpp>

#include <rtc_base/trace_event.h>

namespace wrtc {
    ChannelManager::ChannelManager(
        webrtc::MediaEngineInterface *mediaEngine,
        webrtc::Thread* workerThread,
        webrtc::Thread* networkThread,
        webrtc::Thread* signalingThread
    ): mediaEngine(mediaEngine), signalingThread(signalingThread), workerThread(workerThread), networkThread(networkThread) {
        RTC_DCHECK_RUN_ON(signalingThread);
        RTC_DCHECK(workerThread);
        RTC_DCHECK(networkThread);
    }

    std::unique_ptr<webrtc::VoiceChannel> ChannelManager::CreateVoiceChannel(
        webrtc::Call *call,
        const webrtc::MediaConfig &mediaConfig,
        const std::string &mid,
        const bool srtpRequired,
        const webrtc::CryptoOptions &cryptoOptions,
        const webrtc::AudioOptions &options
    ) {
        RTC_DCHECK(call);
        RTC_DCHECK(mediaEngine);
        if (!workerThread->IsCurrent()) {
            std::unique_ptr<webrtc::VoiceChannel> temp;
            workerThread->BlockingCall([&] {
                temp = CreateVoiceChannel(call, mediaConfig, mid, srtpRequired, cryptoOptions, options);
            });
            return std::move(temp);
        }
        RTC_DCHECK_RUN_ON(workerThread);
        auto sendMediaChannel = mediaEngine->voice().CreateSendChannel(
            call,
            mediaConfig,
            options,
            cryptoOptions,
            webrtc::AudioCodecPairId::Create()
        );
        if (!sendMediaChannel) {
            return nullptr;
        }
        auto receiveMediaChannel = mediaEngine->voice().CreateReceiveChannel(
            call,
            mediaConfig,
            options,
            cryptoOptions,
            webrtc::AudioCodecPairId::Create()
        );
        if (!receiveMediaChannel) {
            return nullptr;
        }
        return std::make_unique<webrtc::VoiceChannel>(
            workerThread,
            networkThread,
            signalingThread,
            std::move(sendMediaChannel),
            std::move(receiveMediaChannel),
            mid,
            srtpRequired,
            cryptoOptions,
            &ssrcGenerator
        );
    }

    std::unique_ptr<webrtc::VideoChannel> ChannelManager::CreateVideoChannel(
        webrtc::Call *call,
        const webrtc::MediaConfig &mediaConfig,
        const std::string &mid,
        const bool srtpRequired,
        const webrtc::CryptoOptions &cryptoOptions,
        const webrtc::VideoOptions &options,
        webrtc::VideoBitrateAllocatorFactory *bitrateAllocatorFactory
    ) {
        RTC_DCHECK(call);
        RTC_DCHECK(mediaEngine);
        if (!workerThread->IsCurrent()) {
            std::unique_ptr<webrtc::VideoChannel> temp = nullptr;
            workerThread->BlockingCall([&] {
              temp = CreateVideoChannel(
                  call,
                  mediaConfig,
                  mid,
                  srtpRequired,
                  cryptoOptions,
                  options,
                  bitrateAllocatorFactory
              );
            });
            return temp;
        }
        RTC_DCHECK_RUN_ON(workerThread);
        std::unique_ptr<webrtc::VideoMediaSendChannelInterface> sendMediaChannel = mediaEngine->video().CreateSendChannel(
            call,
            mediaConfig,
            options,
            cryptoOptions,
            bitrateAllocatorFactory
        );
        if (!sendMediaChannel) {
            return nullptr;
        }
        std::unique_ptr<webrtc::VideoMediaReceiveChannelInterface> receiveMediaChannel = mediaEngine->video().CreateReceiveChannel(
            call,
            mediaConfig,
            options,
            cryptoOptions
        );
        return std::make_unique<webrtc::VideoChannel>(
            workerThread,
            networkThread,
            signalingThread,
            std::move(sendMediaChannel),
            std::move(receiveMediaChannel),
            mid,
            srtpRequired,
            cryptoOptions,
            &ssrcGenerator
        );
    }

} // wrtc