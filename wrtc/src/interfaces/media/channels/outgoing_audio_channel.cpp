//
// Created by Laky64 on 31/03/2024.
//

#include <wrtc/interfaces/media/channels/outgoing_audio_channel.hpp>

#include <wrtc/interfaces/native_connection.hpp>

namespace wrtc {
    OutgoingAudioChannel::OutgoingAudioChannel(
        webrtc::Call *call,
        ChannelManager *channelManager,
        webrtc::RtpTransport* rtpTransport,
        const MediaContent& mediaContent,
        webrtc::Thread *workerThread,
        webrtc::Thread* networkThread,
        webrtc::LocalAudioSinkAdapter* sink
    ): _ssrc(mediaContent.ssrc), workerThread(workerThread), networkThread(networkThread), sink(sink) {
        webrtc::AudioOptions audioOptions;
        audioOptions.echo_cancellation = false;
        audioOptions.noise_suppression = false;
        audioOptions.auto_gain_control = false;
        audioOptions.highpass_filter = false;

        channel = channelManager->CreateVoiceChannel(
            call,
            webrtc::MediaConfig(),
            std::to_string(_ssrc),
            false,
            NativeNetworkInterface::getDefaultCryptoOptions(),
            audioOptions
        );
        networkThread->BlockingCall([&] {
            channel->SetRtpTransport(rtpTransport);
        });
        std::vector<webrtc::Codec> codecs;
        for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : mediaContent.payloadTypes) {
            if (name == "opus") {
                webrtc::Codec codec = webrtc::CreateAudioCodec(static_cast<int>(id), name, static_cast<int>(clockrate), channels);
                codec.SetParam(webrtc::kCodecParamUseInbandFec, 1);
                codec.SetParam(webrtc::kCodecParamPTime, 60);
                for (const auto &[type, subtype] : feedbackTypes) {
                    codec.AddFeedbackParam(webrtc::FeedbackParam(type, subtype));
                }
                codecs.push_back(std::move(codec));
                break;
            }
        }
        const auto outgoingDescription = std::make_unique<webrtc::AudioContentDescription>();
        for (const auto &rtpExtension : mediaContent.rtpExtensions) {
            outgoingDescription->AddRtpHeaderExtension(webrtc::RtpExtension(rtpExtension.uri, rtpExtension.id));
        }
        outgoingDescription->set_rtcp_mux(true);
        outgoingDescription->set_rtcp_reduced_size(true);
        outgoingDescription->set_direction(webrtc::RtpTransceiverDirection::kSendOnly);
        outgoingDescription->set_codecs(codecs);
        outgoingDescription->set_bandwidth(-1);
        outgoingDescription->AddStream(webrtc::StreamParams::CreateLegacy(_ssrc));
        const auto incomingDescription = std::make_unique<webrtc::AudioContentDescription>();
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
        set_enabled(true);
        workerThread->BlockingCall([&] {
            webrtc::RtpParameters initialParameters = channel->send_channel()->GetRtpSendParameters(_ssrc);
            webrtc::RtpParameters updatedParameters = initialParameters;
            if (updatedParameters.encodings.empty()) {
                updatedParameters.encodings.emplace_back();
            }
            if (initialParameters != updatedParameters) {
                channel->send_channel()->SetRtpSendParameters(_ssrc, updatedParameters);
            }
        });
    }

    void OutgoingAudioChannel::set_enabled(const bool enable) const {
        channel->Enable(enable);
        workerThread->BlockingCall([&] {
            channel->send_channel()->SetAudioSend(_ssrc, enable, nullptr, sink);
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
        sink = nullptr;
    }

    uint32_t OutgoingAudioChannel::ssrc() const {
        return _ssrc;
    }
} // wrtc