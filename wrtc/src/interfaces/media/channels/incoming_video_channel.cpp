//
// Created by Lauren on 25/10/24.
//

#include <api/video/builtin_video_bitrate_allocator_factory.h>
#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/media/channels/incoming_video_channel.hpp>
#include <wrtc/models/outgoing_video_format.hpp>

namespace wrtc::interfaces::media::channels {
    IncomingVideoChannel::IncomingVideoChannel(
        webrtc::Call* call,
        ChannelManager* channel_manager,
        webrtc::RtpTransport* rtp_transport,
        std::vector<models::SsrcGroup> ssrc_groups,
        webrtc::UniqueRandomIdGenerator* random_id_generator,
        const std::vector<webrtc::Codec>& codecs,
        utils::SafeThread& worker_thread,
        utils::SafeThread& network_thread,
        std::weak_ptr<RemoteVideoSink> remote_video_sink,
        const std::map<int32_t, FrameTransformer::PayloadType>& payload_type_mapping,
        E2EEncryptor* encryptor
    ): worker_thread_(worker_thread), network_thread_(network_thread) {
        sink_ = std::make_unique<RawVideoSink>();
        const uint32_t mid = random_id_generator->GenerateId();
        const auto stream_id = "video" + std::to_string(mid);
        video_bitrate_allocator_factory_ = webrtc::CreateBuiltinVideoBitrateAllocatorFactory();

        channel_ = channel_manager->create_video_channel(
            call,
            webrtc::MediaConfig(),
            stream_id,
            false,
            NativeNetworkInterface::get_default_crypto_options(),
            webrtc::VideoOptions(),
            video_bitrate_allocator_factory_.get()
        );

        network_thread.BlockingCall([&] {
            channel_->SetRtpTransport(rtp_transport);
        });

        auto outgoing_video_description = std::make_unique<webrtc::VideoContentDescription>();
        outgoing_video_description->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kAudioLevelUri, 1));
        outgoing_video_description->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kAbsSendTimeUri, 2));
        outgoing_video_description->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kTransportSequenceNumberUri, 3));
        outgoing_video_description->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kVideoRotationUri, 13));
        outgoing_video_description->set_rtcp_mux(true);
        outgoing_video_description->set_rtcp_reduced_size(true);
        outgoing_video_description->set_direction(webrtc::RtpTransceiverDirection::kRecvOnly);
        outgoing_video_description->set_codecs(codecs);
        outgoing_video_description->set_bandwidth(-1);

        std::vector<uint32_t> all_ssrcs;
        webrtc::StreamParams video_recv_stream_params;
        for (const auto& [semantics, ssrcs] : ssrc_groups) {
            for (auto ssrc : ssrcs) {
                if (std::ranges::find(all_ssrcs, ssrc) == all_ssrcs.end()) {
                    all_ssrcs.push_back(ssrc);
                }
            }

            if (semantics == "SIM") {
                if (ssrc_ == 0) {
                    ssrc_ = ssrcs[0];
                }
            }

            const webrtc::SsrcGroup parsed_group(semantics, ssrcs);
            video_recv_stream_params.ssrc_groups.push_back(parsed_group);
        }

        if (ssrc_ == 0 && ssrc_groups.size() == 1) {
            ssrc_ = ssrc_groups[0].ssrcs[0];
        }
        video_recv_stream_params.ssrcs = all_ssrcs;

        video_recv_stream_params.cname = "cname";
        video_recv_stream_params.set_stream_ids({stream_id});

        auto incoming_video_description = std::make_unique<webrtc::VideoContentDescription>();
        incoming_video_description->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kAudioLevelUri, 1));
        incoming_video_description->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kAbsSendTimeUri, 2));
        incoming_video_description->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kTransportSequenceNumberUri, 3));
        incoming_video_description->AddRtpHeaderExtension(webrtc::RtpExtension(webrtc::RtpExtension::kVideoRotationUri, 13));
        incoming_video_description->set_rtcp_mux(true);
        incoming_video_description->set_rtcp_reduced_size(true);
        incoming_video_description->set_direction(webrtc::RtpTransceiverDirection::kSendOnly);
        incoming_video_description->set_codecs(codecs);
        incoming_video_description->set_bandwidth(-1);

        incoming_video_description->AddStream(video_recv_stream_params);

        worker_thread.BlockingCall([&] {
            channel_->rtp_transport()->SetActivePayloadTypeDemuxing(false);
            channel_->SetLocalContent(outgoing_video_description.get(), webrtc::SdpType::kOffer);
            channel_->SetRemoteContent(incoming_video_description.get(), webrtc::SdpType::kAnswer);

            channel_->video_media_receive_channel()->SetSink(ssrc_, sink_.get());

            sink_->set_remote_video_sink(ssrc_, [remote_video_sink](const uint32_t ssrc, std::unique_ptr<webrtc::VideoFrame> frame) {
                if (const auto sink = remote_video_sink.lock()) {
                    sink->send_frame(ssrc, std::move(frame));
                }
            });

            if (encryptor) {
                channel_->video_media_receive_channel()->SetDepacketizerToDecoderFrameTransformer(
                    ssrc_,
                    webrtc::make_ref_counted<FrameTransformer>(
                        false,
                        encryptor,
                        ssrc_,
                        payload_type_mapping,
                        nullptr,
                        nullptr
                    )
                );
            }
        });
        channel_->Enable(true);
    }

    IncomingVideoChannel::~IncomingVideoChannel() {
        channel_->Enable(false);
        network_thread_.BlockingCall([&] {
            channel_->SetRtpTransport(nullptr);
        });
        worker_thread_.BlockingCall([&] {
            channel_->video_media_receive_channel()->SetDepacketizerToDecoderFrameTransformer(ssrc_, nullptr);
            channel_->video_media_receive_channel()->SetSink(ssrc_, nullptr);
            channel_ = nullptr;
            sink_ = nullptr;
        });
    }

    uint32_t IncomingVideoChannel::ssrc() const {
        return ssrc_;
    }
} // wrtc::interfaces::media::channels
