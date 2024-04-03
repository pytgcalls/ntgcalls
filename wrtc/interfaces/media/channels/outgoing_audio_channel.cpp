//
// Created by Laky64 on 31/03/2024.
//

#include "outgoing_audio_channel.hpp"

#include "wrtc/interfaces/native_connection.hpp"

namespace wrtc {
    OutgoingAudioChannel::OutgoingAudioChannel(
        webrtc::Call *call,
        ChannelManager *channelManager,
        webrtc::RtpTransport* rtpTransport,
        const MediaContent& mediaContent,
        rtc::Thread *workerThread,
        rtc::Thread* networkThread,
        webrtc::LocalAudioSinkAdapter* sink
    ): _ssrc(mediaContent.ssrc), workerThread(workerThread), networkThread(networkThread) {
        cricket::AudioOptions audioOptions;
        audioOptions.echo_cancellation = false;
        audioOptions.noise_suppression = false;
        audioOptions.auto_gain_control = false;
        audioOptions.highpass_filter = false;

        const auto contentId = std::to_string(_ssrc);
        channel = channelManager->CreateVoiceChannel(
            call,
            cricket::MediaConfig(),
            contentId,
            false,
            NativeConnection::getDefaultCryptoOptions(),
            audioOptions
        );
        networkThread->BlockingCall([&] {
            channel->SetRtpTransport(rtpTransport);
        });
        std::vector<cricket::AudioCodec> codecs;
        for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : mediaContent.payloadTypes) {
            if (name == "opus") {
                cricket::AudioCodec codec = cricket::CreateAudioCodec(static_cast<int>(id), name, static_cast<int>(clockrate), channels);
                codec.SetParam(cricket::kCodecParamUseInbandFec, 1);
                codec.SetParam(cricket::kCodecParamPTime, 60);
                for (const auto &[type, subtype] : feedbackTypes) {
                    codec.AddFeedbackParam(cricket::FeedbackParam(type, subtype));
                }
                codecs.push_back(std::move(codec));
                break;
            }
        }
        const auto outgoingDescription = std::make_unique<cricket::AudioContentDescription>();
        for (const auto &rtpExtension : mediaContent.rtpExtensions) {
            outgoingDescription->AddRtpHeaderExtension(webrtc::RtpExtension(rtpExtension.uri, rtpExtension.id));
        }
        outgoingDescription->set_rtcp_mux(true);
        outgoingDescription->set_rtcp_reduced_size(true);
        outgoingDescription->set_direction(webrtc::RtpTransceiverDirection::kSendOnly);
        outgoingDescription->set_codecs(codecs);
        outgoingDescription->set_bandwidth(-1);
        outgoingDescription->AddStream(cricket::StreamParams::CreateLegacy(_ssrc));
        const auto incomingDescription = std::make_unique<cricket::AudioContentDescription>();
        for (const auto &rtpExtension : mediaContent.rtpExtensions) {
            incomingDescription->AddRtpHeaderExtension(webrtc::RtpExtension(rtpExtension.uri, rtpExtension.id));
        }
        incomingDescription->set_rtcp_mux(true);
        incomingDescription->set_rtcp_reduced_size(true);
        incomingDescription->set_direction(webrtc::RtpTransceiverDirection::kRecvOnly);
        incomingDescription->set_codecs(codecs);
        incomingDescription->set_bandwidth(-1);
        workerThread->BlockingCall([&] {
            channel->SetPayloadTypeDemuxingEnabled(false);
            std::string errorDesc;
            channel->SetLocalContent(outgoingDescription.get(), webrtc::SdpType::kOffer, errorDesc);
            channel->SetRemoteContent(incomingDescription.get(), webrtc::SdpType::kAnswer, errorDesc);
        });
        channel->Enable(true);
        workerThread->BlockingCall([&] {
            channel->send_channel()->SetAudioSend(_ssrc, true, nullptr, sink);
            webrtc::RtpParameters initialParameters = channel->send_channel()->GetRtpSendParameters(_ssrc);
               webrtc::RtpParameters updatedParameters = initialParameters;
               if (updatedParameters.encodings.empty()) {
                   updatedParameters.encodings.emplace_back();
               }
               updatedParameters.encodings[0].max_bitrate_bps = 32 * 1024;
               if (initialParameters != updatedParameters) {
                   channel->send_channel()->SetRtpSendParameters(_ssrc, updatedParameters);
               }
        });
    }

    OutgoingAudioChannel::~OutgoingAudioChannel() {
        channel->Enable(false);
        networkThread->BlockingCall([&] {
            channel->SetRtpTransport(nullptr);
        });
        workerThread->BlockingCall([&] {
            channel = nullptr;
        });
    }

    uint32_t OutgoingAudioChannel::ssrc() const {
        return _ssrc;
    }
} // wrtc