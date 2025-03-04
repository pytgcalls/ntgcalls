//
// Created by Laky64 on 02/04/2024.
//

#include <wrtc/interfaces/media/channels/outgoing_video_channel.hpp>

#include <wrtc/interfaces/native_connection.hpp>
#include <api/video/builtin_video_bitrate_allocator_factory.h>

namespace wrtc {
    OutgoingVideoChannel::OutgoingVideoChannel(
        webrtc::Call *call,
        ChannelManager *channelManager,
        webrtc::RtpTransport *rtpTransport,
        const MediaContent &mediaContent,
        rtc::Thread *workerThread,
        rtc::Thread *networkThread,
        LocalVideoAdapter* sink
    ): _ssrc(mediaContent.ssrc), workerThread(workerThread), networkThread(networkThread), sink(sink) {
        cricket::VideoOptions videoOptions;
        videoOptions.is_screencast = mediaContent.isScreenCast();
        bitrateAllocatorFactory = webrtc::CreateBuiltinVideoBitrateAllocatorFactory();
        channel = channelManager->CreateVideoChannel(
            call,
            cricket::MediaConfig(),
            std::to_string(_ssrc),
            false,
            NativeNetworkInterface::getDefaultCryptoOptions(),
            videoOptions,
            bitrateAllocatorFactory.get()
        );
        networkThread->BlockingCall([&] {
            channel->SetRtpTransport(rtpTransport);
        });
        std::vector<cricket::Codec> unsortedCodecs;
        for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : mediaContent.payloadTypes) {
            cricket::Codec codec = cricket::CreateVideoCodec(static_cast<int>(id), name);
            for (const auto &[fst, snd] : parameters) {
                codec.SetParam(fst, snd);
            }
            for (const auto &[type, subtype] : feedbackTypes) {
                codec.AddFeedbackParam(cricket::FeedbackParam(type, subtype));
            }
            unsortedCodecs.push_back(std::move(codec));
        }
        const std::vector<std::string> codecPreferences = {
            cricket::kH264CodecName
        };
        std::vector<cricket::Codec> codecs;
        for (const auto &name : codecPreferences) {
            for (const auto &codec : unsortedCodecs) {
                if (codec.name == name) {
                    codecs.push_back(codec);
                }
            }
        }
        for (const auto &codec : unsortedCodecs) {
            if (std::ranges::find(codecs, codec) == codecs.end()) {
                codecs.push_back(codec);
            }
        }

        auto outgoingVideoDescription = std::make_unique<cricket::VideoContentDescription>();
        for (const auto &rtpExtension : mediaContent.rtpExtensions) {
            outgoingVideoDescription->AddRtpHeaderExtension(rtpExtension);
        }
        outgoingVideoDescription->set_rtcp_mux(true);
        outgoingVideoDescription->set_rtcp_reduced_size(true);
        outgoingVideoDescription->set_direction(webrtc::RtpTransceiverDirection::kSendOnly);
        outgoingVideoDescription->set_codecs(codecs);
        outgoingVideoDescription->set_bandwidth(-1);
        cricket::StreamParams videoSendStreamParams;
        for (const auto &[semantics, ssrcs] : mediaContent.ssrcGroups) {
            for (auto ssrc : ssrcs) {
                if (!videoSendStreamParams.has_ssrc(ssrc)) {
                    videoSendStreamParams.ssrcs.push_back(ssrc);
                }
            }
            cricket::SsrcGroup mappedGroup(semantics, ssrcs);
            videoSendStreamParams.ssrc_groups.push_back(std::move(mappedGroup));
        }
        videoSendStreamParams.cname = "cname";
        outgoingVideoDescription->AddStream(videoSendStreamParams);

        auto incomingVideoDescription = std::make_unique<cricket::VideoContentDescription>();
        for (const auto &rtpExtension : mediaContent.rtpExtensions) {
            incomingVideoDescription->AddRtpHeaderExtension(webrtc::RtpExtension(rtpExtension.uri, rtpExtension.id));
        }
        incomingVideoDescription->set_rtcp_mux(true);
        incomingVideoDescription->set_rtcp_reduced_size(true);
        incomingVideoDescription->set_direction(webrtc::RtpTransceiverDirection::kRecvOnly);
        incomingVideoDescription->set_codecs(codecs);
        incomingVideoDescription->set_bandwidth(-1);
        workerThread->BlockingCall([&] {
            channel->SetPayloadTypeDemuxingEnabled(false);
            std::string errorDesc;
            channel->SetLocalContent(outgoingVideoDescription.get(), webrtc::SdpType::kOffer, errorDesc);
            channel->SetRemoteContent(incomingVideoDescription.get(), webrtc::SdpType::kAnswer, errorDesc);
        });
        channel->Enable(true);
        set_enabled(true);
        workerThread->BlockingCall([&] {
            webrtc::RtpParameters rtpParameters = channel->send_channel()->GetRtpSendParameters(_ssrc);
            rtpParameters.degradation_preference = webrtc::DegradationPreference::MAINTAIN_RESOLUTION;
            channel->send_channel()->SetRtpSendParameters(_ssrc, rtpParameters);
        });
    }

    OutgoingVideoChannel::~OutgoingVideoChannel() {
        channel->Enable(false);
        networkThread->BlockingCall([&] {
            channel->SetRtpTransport(nullptr);
        });
        workerThread->BlockingCall([&] {
            channel = nullptr;
            bitrateAllocatorFactory = nullptr;
        });
        sink = nullptr;
    }

    void OutgoingVideoChannel::set_enabled(const bool enable) const {
        channel->Enable(enable);
        workerThread->BlockingCall([&] {
            channel->send_channel()->SetVideoSend(_ssrc, nullptr, enable ? sink:nullptr);
        });
    }

    uint32_t OutgoingVideoChannel::ssrc() const {
        return _ssrc;
    }
} // wrtc