//
// Created by Laky64 on 03/10/24.
//

#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/media/raw_audio_sink.hpp>
#include <wrtc/interfaces/media/channels/incoming_audio_channel.hpp>

namespace wrtc {
    IncomingAudioChannel::IncomingAudioChannel(
        webrtc::Call* call,
        ChannelManager *channelManager,
        webrtc::RtpTransport* rtpTransport,
        const MediaContent& mediaContent,
        rtc::Thread *workerThread,
        rtc::Thread* networkThread,
        std::weak_ptr<RemoteAudioSink> remoteAudioSink
    ): _ssrc(mediaContent.ssrc), workerThread(workerThread), networkThread(networkThread) {
        updateActivity();

        const auto streamId = std::to_string(_ssrc);

        cricket::AudioOptions audioOptions;
        audioOptions.audio_jitter_buffer_fast_accelerate = true;
        audioOptions.audio_jitter_buffer_min_delay_ms = 50;

        channel = channelManager->CreateVoiceChannel(
            call,
            cricket::MediaConfig(),
            streamId,
            false,
            NativeNetworkInterface::getDefaultCryptoOptions(),
            audioOptions
        );
        networkThread->BlockingCall([&] {
           channel->SetRtpTransport(rtpTransport);
        });
        std::vector<cricket::Codec> codecs;
        for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : mediaContent.payloadTypes) {
            cricket::Codec codec = cricket::CreateAudioCodec(static_cast<int>(id), name, static_cast<int>(clockrate), channels);
            codec.SetParam(cricket::kCodecParamUseInbandFec, 1);
            codec.SetParam(cricket::kCodecParamPTime, 60);
            for (const auto &[type, subtype] : feedbackTypes) {
                codec.AddFeedbackParam(cricket::FeedbackParam(type, subtype));
            }
            codecs.push_back(std::move(codec));
            break;
        }

        const auto outgoingDescription = std::make_unique<cricket::AudioContentDescription>();
        for (const auto &rtpExtension : mediaContent.rtpExtensions) {
            outgoingDescription->AddRtpHeaderExtension(webrtc::RtpExtension(rtpExtension.uri, rtpExtension.id));
        }
        outgoingDescription->set_rtcp_mux(true);
        outgoingDescription->set_rtcp_reduced_size(true);
        outgoingDescription->set_direction(webrtc::RtpTransceiverDirection::kRecvOnly);
        outgoingDescription->set_codecs(codecs);
        outgoingDescription->set_bandwidth(-1);

        const auto incomingDescription = std::make_unique<cricket::AudioContentDescription>();
        for (const auto &rtpExtension : mediaContent.rtpExtensions) {
            incomingDescription->AddRtpHeaderExtension(webrtc::RtpExtension(rtpExtension.uri, rtpExtension.id));
        }
        incomingDescription->set_rtcp_mux(true);
        incomingDescription->set_rtcp_reduced_size(true);
        incomingDescription->set_direction(webrtc::RtpTransceiverDirection::kSendOnly);
        incomingDescription->set_codecs(codecs);
        incomingDescription->set_bandwidth(-1);

        cricket::StreamParams streamParams = cricket::StreamParams::CreateLegacy(mediaContent.ssrc);
        streamParams.set_stream_ids({ streamId });
        incomingDescription->AddStream(streamParams);

        workerThread->BlockingCall([&] {
            channel->SetPayloadTypeDemuxingEnabled(true);
            std::string errorDesc;
            channel->SetLocalContent(outgoingDescription.get(), webrtc::SdpType::kOffer, errorDesc);
            channel->SetRemoteContent(incomingDescription.get(), webrtc::SdpType::kAnswer, errorDesc);
        });
        channel->Enable(true);
        workerThread->BlockingCall([&] {
            auto rawSink = std::make_unique<RawAudioSink>();
            rawSink->setRemoteAudioSink(_ssrc, [remoteAudioSink](std::unique_ptr<AudioFrame> frame) {
                if (const auto remoteAudio = remoteAudioSink.lock()) {
                    remoteAudio->sendData(std::move(frame));
                }
            });
            channel->receive_channel()->SetRawAudioSink(_ssrc, std::move(rawSink));
        });
    }

    IncomingAudioChannel::~IncomingAudioChannel() {
        channel->Enable(false);
        networkThread->BlockingCall([&] {
           channel->SetRtpTransport(nullptr);
        });
        workerThread->BlockingCall([&] {
            channel = nullptr;
        });
    }

    void IncomingAudioChannel::updateActivity() {
        activityTimestamp = rtc::TimeMillis();
    }

    int64_t IncomingAudioChannel::getActivity() const {
        return activityTimestamp;
    }

    uint32_t IncomingAudioChannel::ssrc() const {
        return _ssrc;
    }
} // wrtc