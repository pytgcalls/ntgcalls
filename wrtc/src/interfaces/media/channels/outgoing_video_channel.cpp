//
// Created by Lauren on 02/04/24.
//

#include <set>
#include <api/video/builtin_video_bitrate_allocator_factory.h>
#include <wrtc/interfaces/native_connection.hpp>
#include <wrtc/interfaces/media/channels/outgoing_video_channel.hpp>

namespace wrtc::interfaces::media::channels {
    OutgoingVideoChannel::OutgoingVideoChannel(
        webrtc::Call* call,
        ChannelManager* channel_manager,
        webrtc::RtpTransport* rtp_transport,
        const models::MediaContent& media_content,
        utils::SafeThread& worker_thread,
        utils::SafeThread& network_thread,
        LocalVideoAdapter* sink,
        const std::map<int32_t, FrameTransformer::PayloadType>& payload_type_mapping,
        E2EEncryptor* encryptor
    ): ssrc_(media_content.ssrc), worker_thread_(worker_thread), network_thread_(network_thread), sink_(sink) {
        webrtc::VideoOptions video_options;
        video_options.is_screencast = media_content.is_screen_cast();
        bitrate_allocator_factory_ = webrtc::CreateBuiltinVideoBitrateAllocatorFactory();
        channel_ = channel_manager->create_video_channel(
            call,
            webrtc::MediaConfig(),
            std::to_string(ssrc_),
            false,
            NativeNetworkInterface::get_default_crypto_options(),
            video_options,
            bitrate_allocator_factory_.get()
        );
        network_thread.BlockingCall([&] {
            channel_->SetRtpTransport(rtp_transport);
        });
        std::vector<webrtc::Codec> unsorted_codecs;
        for (const auto& [id, name, clockrate, channels, feedbackTypes, parameters] : media_content.payload_types) {
            webrtc::Codec codec = webrtc::CreateVideoCodec(id, name);
            for (const auto& [fst, snd] : parameters) {
                codec.SetParam(fst, snd);
            }
            for (const auto& [type, subtype] : feedbackTypes) {
                codec.AddFeedbackParam(webrtc::FeedbackParam(type, subtype));
            }
            unsorted_codecs.push_back(std::move(codec));
        }
        const std::vector<std::string> codec_preferences = {
            webrtc::kH264CodecName
        };
        std::vector<webrtc::Codec> codecs;
        for (const auto& name : codec_preferences) {
            for (const auto& codec : unsorted_codecs) {
                if (codec.name == name) {
                    codecs.push_back(codec);
                }
            }
        }
        for (const auto& codec : unsorted_codecs) {
            if (std::ranges::find(codecs, codec) == codecs.end()) {
                codecs.push_back(codec);
            }
        }

        auto outgoing_video_description = std::make_unique<webrtc::VideoContentDescription>();
        for (const auto& rtp_extension : media_content.rtp_extensions) {
            outgoing_video_description->AddRtpHeaderExtension(rtp_extension);
        }
        outgoing_video_description->set_rtcp_mux(true);
        outgoing_video_description->set_rtcp_reduced_size(true);
        outgoing_video_description->set_direction(webrtc::RtpTransceiverDirection::kSendOnly);
        outgoing_video_description->set_codecs(codecs);
        outgoing_video_description->set_bandwidth(-1);
        webrtc::StreamParams video_send_stream_params;
        for (const auto& [semantics, ssrcs] : media_content.ssrc_groups) {
            for (auto ssrc : ssrcs) {
                if (!video_send_stream_params.has_ssrc(ssrc)) {
                    video_send_stream_params.ssrcs.push_back(ssrc);
                }
            }
            webrtc::SsrcGroup mapped_group(semantics, ssrcs);
            video_send_stream_params.ssrc_groups.push_back(std::move(mapped_group));
        }
        video_send_stream_params.cname = "cname";
        outgoing_video_description->AddStream(video_send_stream_params);

        auto incoming_video_description = std::make_unique<webrtc::VideoContentDescription>();
        for (const auto& rtp_extension : media_content.rtp_extensions) {
            incoming_video_description->AddRtpHeaderExtension(webrtc::RtpExtension(rtp_extension.uri, rtp_extension.id));
        }
        incoming_video_description->set_rtcp_mux(true);
        incoming_video_description->set_rtcp_reduced_size(true);
        incoming_video_description->set_direction(webrtc::RtpTransceiverDirection::kRecvOnly);
        incoming_video_description->set_codecs(codecs);
        incoming_video_description->set_bandwidth(-1);
        worker_thread.BlockingCall([&] {
            channel_->rtp_transport()->SetActivePayloadTypeDemuxing(false);
            channel_->SetLocalContent(outgoing_video_description.get(), webrtc::SdpType::kOffer);
            channel_->SetRemoteContent(incoming_video_description.get(), webrtc::SdpType::kAnswer);
        });
        channel_->Enable(true);
        set_enabled(true);
        worker_thread.BlockingCall([&] {
            webrtc::RtpParameters rtp_parameters = channel_->video_media_send_channel()->GetRtpSendParameters(ssrc_);
            rtp_parameters.degradation_preference = webrtc::DegradationPreference::MAINTAIN_RESOLUTION;
            channel_->video_media_send_channel()->SetRtpSendParameters(ssrc_, rtp_parameters);

            if (encryptor) {
                std::set<uint32_t> transformer_ssrcs;
                transformer_ssrcs.insert(ssrc_);
                for (const auto& [semantics, ssrcs] : media_content.ssrc_groups) {
                    if (semantics == "SIM") {
                        for (const auto& ssrc : ssrcs) {
                            transformer_ssrcs.insert(ssrc);
                        }
                    }
                }
                for (const auto& ssrc : transformer_ssrcs) {
                    channel_->video_media_send_channel()->SetEncoderToPacketizerFrameTransformer(
                        ssrc,
                        webrtc::make_ref_counted<FrameTransformer>(
                            true,
                            encryptor,
                            int64_t(),
                            payload_type_mapping,
                            nullptr,
                            nullptr
                        )
                    );
                }
            }
        });
    }

    OutgoingVideoChannel::~OutgoingVideoChannel() {
        channel_->Enable(false);
        network_thread_.BlockingCall([&] {
            channel_->SetRtpTransport(nullptr);
        });
        worker_thread_.BlockingCall([&] {
            channel_ = nullptr;
            bitrate_allocator_factory_ = nullptr;
        });
        sink_ = nullptr;
    }

    void OutgoingVideoChannel::set_enabled(const bool enable) const {
        channel_->Enable(enable);
        worker_thread_.BlockingCall([&] {
            channel_->video_media_send_channel()->SetVideoSend(ssrc_, nullptr, enable ? sink_ : nullptr);
        });
    }

    uint32_t OutgoingVideoChannel::ssrc() const {
        return ssrc_;
    }
} // wrtc::interfaces::media::channels
