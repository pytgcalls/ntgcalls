//
// Created by Laky64 on 25/10/24.
//

#include <api/video/builtin_video_bitrate_allocator_factory.h>
#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/media/channels/incoming_video_channel.hpp>

namespace wrtc {
    IncomingVideoChannel::IncomingVideoChannel(
        webrtc::Call* call,
        ChannelManager* channelManager,
        webrtc::RtpTransport* rtpTransport,
        const MediaContent& mediaContent,
        rtc::Thread* workerThread,
        rtc::Thread* networkThread,
        std::weak_ptr<RemoteVideoSink> remoteVideoSink
    ) : _ssrc(mediaContent.ssrc), workerThread(workerThread), networkThread(networkThread) {
        sink = std::make_unique<RawVideoSink>();

        const auto streamId = std::to_string(_ssrc);
        videoBitrateAllocatorFactory = webrtc::CreateBuiltinVideoBitrateAllocatorFactory();

        channel = channelManager->CreateVideoChannel(
            call,
            cricket::MediaConfig(),
            streamId,
            false,
            NativeNetworkInterface::getDefaultCryptoOptions(),
            cricket::VideoOptions(),
            videoBitrateAllocatorFactory.get()
        );

        networkThread->BlockingCall([&] {
            channel->SetRtpTransport(rtpTransport);
        });

        std::vector<cricket::Codec> codecs;
        for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : mediaContent.payloadTypes) {
            cricket::Codec codec = cricket::CreateVideoCodec(static_cast<int>(id), name);
            for (const auto & [fst, snd] : parameters) {
                codec.SetParam(fst, snd);
            }
            for (const auto & [type, subtype] : feedbackTypes) {
                codec.AddFeedbackParam(cricket::FeedbackParam(type, subtype));
            }
            codecs.push_back(std::move(codec));
        }

        auto outgoingVideoDescription = std::make_unique<cricket::VideoContentDescription>();
        for (const auto &rtpExtension : mediaContent.rtpExtensions) {
            outgoingVideoDescription->AddRtpHeaderExtension(webrtc::RtpExtension(rtpExtension.uri, rtpExtension.id));
        }
        outgoingVideoDescription->set_rtcp_mux(true);
        outgoingVideoDescription->set_rtcp_reduced_size(true);
        outgoingVideoDescription->set_direction(webrtc::RtpTransceiverDirection::kRecvOnly);
        outgoingVideoDescription->set_codecs(codecs);
        outgoingVideoDescription->set_bandwidth(-1);

        std::vector<uint32_t> allSsrcs;
        cricket::StreamParams videoRecvStreamParams;
        for (const auto & [ssrcs, semantics] : mediaContent.ssrcGroups) {
            for (auto ssrc : ssrcs) {
                if (std::ranges::find(allSsrcs, ssrc) == allSsrcs.end()) {
                    allSsrcs.push_back(ssrc);
                }
            }

            cricket::SsrcGroup parsedGroup(semantics, ssrcs);
            videoRecvStreamParams.ssrc_groups.push_back(parsedGroup);
        }
        videoRecvStreamParams.ssrcs = allSsrcs;

        videoRecvStreamParams.cname = "cname";
        videoRecvStreamParams.set_stream_ids({ streamId });

        auto incomingVideoDescription = std::make_unique<cricket::VideoContentDescription>();
        for (const auto &rtpExtension : mediaContent.rtpExtensions) {
            incomingVideoDescription->AddRtpHeaderExtension(webrtc::RtpExtension(rtpExtension.uri, rtpExtension.id));
        }
        incomingVideoDescription->set_rtcp_mux(true);
        incomingVideoDescription->set_rtcp_reduced_size(true);
        incomingVideoDescription->set_direction(webrtc::RtpTransceiverDirection::kSendOnly);
        incomingVideoDescription->set_codecs(codecs);
        incomingVideoDescription->set_bandwidth(-1);

        incomingVideoDescription->AddStream(videoRecvStreamParams);

        workerThread->BlockingCall([&] {
            channel->SetPayloadTypeDemuxingEnabled(false);
            std::string errorDesc;
            channel->SetLocalContent(outgoingVideoDescription.get(), webrtc::SdpType::kOffer, errorDesc);
            channel->SetRemoteContent(incomingVideoDescription.get(), webrtc::SdpType::kAnswer, errorDesc);

            channel->receive_channel()->SetSink(_ssrc, sink.get());

            sink->setRemoteVideoSink(_ssrc, [remoteVideoSink](const uint32_t ssrc, std::unique_ptr<webrtc::VideoFrame> frame) {
                if (const auto sink = remoteVideoSink.lock()) {
                    sink->sendFrame(ssrc, std::move(frame));
                }
            });
        });
        channel->Enable(true);
    }

    IncomingVideoChannel::~IncomingVideoChannel() {
        channel->Enable(false);
        networkThread->BlockingCall([&] {
           channel->SetRtpTransport(nullptr);
        });
        workerThread->BlockingCall([&] {
            channel = nullptr;
        });
        sink = nullptr;
    }
} // wrtc