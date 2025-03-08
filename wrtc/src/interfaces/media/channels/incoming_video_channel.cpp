//
// Created by Laky64 on 25/10/24.
//

#include <api/video/builtin_video_bitrate_allocator_factory.h>
#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/media/channels/incoming_video_channel.hpp>
#include <wrtc/models/outgoing_video_format.hpp>

namespace wrtc {
    IncomingVideoChannel::IncomingVideoChannel(
        webrtc::Call* call,
        ChannelManager* channelManager,
        webrtc::RtpTransport* rtpTransport,
        std::vector<SsrcGroup> ssrcGroups,
        rtc::UniqueRandomIdGenerator *randomIdGenerator,
        const std::vector<cricket::Codec>& codecs,
        rtc::Thread* workerThread,
        rtc::Thread* networkThread,
        std::weak_ptr<RemoteVideoSink> remoteVideoSink
    ) : workerThread(workerThread), networkThread(networkThread) {
        sink = std::make_unique<RawVideoSink>();
        uint32_t mid = randomIdGenerator->GenerateId();
        const auto streamId = "video" + std::to_string(mid);
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

        auto outgoingVideoDescription = std::make_unique<cricket::VideoContentDescription>();
        outgoingVideoDescription->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kAbsSendTimeUri, 2));
        outgoingVideoDescription->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kTransportSequenceNumberUri, 3));
        outgoingVideoDescription->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kVideoRotationUri, 13));
        outgoingVideoDescription->set_rtcp_mux(true);
        outgoingVideoDescription->set_rtcp_reduced_size(true);
        outgoingVideoDescription->set_direction(webrtc::RtpTransceiverDirection::kRecvOnly);
        outgoingVideoDescription->set_codecs(codecs);
        outgoingVideoDescription->set_bandwidth(-1);

        std::vector<uint32_t> allSsrcs;
        cricket::StreamParams videoRecvStreamParams;
        for (const auto & [semantics, ssrcs] : ssrcGroups) {
            for (auto ssrc : ssrcs) {
                if (std::ranges::find(allSsrcs, ssrc) == allSsrcs.end()) {
                    allSsrcs.push_back(ssrc);
                }
            }

            if (semantics == "SIM") {
                if (_ssrc == 0) {
                    _ssrc = ssrcs[0];
                }
            }

            cricket::SsrcGroup parsedGroup(semantics, ssrcs);
            videoRecvStreamParams.ssrc_groups.push_back(parsedGroup);
        }

        if (_ssrc == 0 && ssrcGroups.size() == 1) {
            _ssrc = ssrcGroups[0].ssrcs[0];
        }
        videoRecvStreamParams.ssrcs = allSsrcs;

        videoRecvStreamParams.cname = "cname";
        videoRecvStreamParams.set_stream_ids({ streamId });

        auto incomingVideoDescription = std::make_unique<cricket::VideoContentDescription>();
        incomingVideoDescription->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kAbsSendTimeUri, 2));
        incomingVideoDescription->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kTransportSequenceNumberUri, 3));
        incomingVideoDescription->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kVideoRotationUri, 13));
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

    uint32_t IncomingVideoChannel::ssrc() const {
        return _ssrc;
    }
} // wrtc