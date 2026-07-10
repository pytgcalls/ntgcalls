//
// Created by Lauren on 31/03/24.
//

#include <wrtc/interfaces/media/channels/outgoing_audio_channel.hpp>

#include <wrtc/interfaces/native_connection.hpp>

namespace wrtc::interfaces::media::channels {
    OutgoingAudioChannel::OutgoingAudioChannel(
        webrtc::Call* call,
        ChannelManager* channel_manager,
        webrtc::RtpTransport* rtp_transport,
        const models::MediaContent& media_content,
        utils::SafeThread& worker_thread,
        utils::SafeThread& network_thread,
        webrtc::LocalAudioSinkAdapter* sink,
        const std::map<int32_t, FrameTransformer::PayloadType>& payload_type_mapping,
        E2EEncryptor* encryptor,
        const std::function<std::pair<uint8_t, bool>()>& get_audio_level_and_speech
    ): ssrc_(media_content.ssrc), worker_thread_(worker_thread), network_thread_(network_thread), sink_(sink) {
        webrtc::AudioOptions audio_options;
        audio_options.echo_cancellation = false;
        audio_options.noise_suppression = false;
        audio_options.auto_gain_control = false;
        audio_options.highpass_filter = false;

        channel_ = channel_manager->create_voice_channel(
            call,
            webrtc::MediaConfig(),
            std::to_string(ssrc_),
            false,
            NativeNetworkInterface::get_default_crypto_options(),
            audio_options
        );
        network_thread.BlockingCall([&] {
            channel_->SetRtpTransport(rtp_transport);
        });
        std::vector<webrtc::Codec> codecs;
        for (const auto &[id, name, clockrate, channels, feedbackTypes, parameters] : media_content.payload_types) {
            if (name == "opus") {
                webrtc::Codec codec = webrtc::CreateAudioCodec(id, name, clockrate, channels);
                codec.SetParam(webrtc::kCodecParamUseInbandFec, 1);
                codec.SetParam(webrtc::kCodecParamPTime, 60);
                for (const auto &[type, subtype] : feedbackTypes) {
                    codec.AddFeedbackParam(webrtc::FeedbackParam(type, subtype));
                }
                codecs.push_back(std::move(codec));
                break;
            }
        }
        const auto outgoing_description = std::make_unique<webrtc::AudioContentDescription>();
        for (const auto &rtp_extension : media_content.rtp_extensions) {
            outgoing_description->AddRtpHeaderExtension(webrtc::RtpExtension(rtp_extension.uri, rtp_extension.id));
        }
        outgoing_description->set_rtcp_mux(true);
        outgoing_description->set_rtcp_reduced_size(true);
        outgoing_description->set_direction(webrtc::RtpTransceiverDirection::kSendOnly);
        outgoing_description->set_codecs(codecs);
        outgoing_description->set_bandwidth(-1);
        outgoing_description->AddStream(webrtc::StreamParams::CreateLegacy(ssrc_));
        const auto incoming_description = std::make_unique<webrtc::AudioContentDescription>();
        for (const auto &rtp_extension : media_content.rtp_extensions) {
            incoming_description->AddRtpHeaderExtension(webrtc::RtpExtension(rtp_extension.uri, rtp_extension.id));
        }
        incoming_description->set_rtcp_mux(true);
        incoming_description->set_rtcp_reduced_size(true);
        incoming_description->set_direction(webrtc::RtpTransceiverDirection::kRecvOnly);
        incoming_description->set_codecs(codecs);
        incoming_description->set_bandwidth(-1);
        worker_thread.BlockingCall([&] {
            channel_->rtp_transport()->SetActivePayloadTypeDemuxing(false);
            channel_->SetLocalContent(outgoing_description.get(), webrtc::SdpType::kOffer);
            channel_->SetRemoteContent(incoming_description.get(), webrtc::SdpType::kAnswer);
        });
        set_enabled(true);
        worker_thread.BlockingCall([&] {
            const webrtc::RtpParameters initial_parameters = channel_->voice_media_send_channel()->GetRtpSendParameters(ssrc_);
            webrtc::RtpParameters updated_parameters = initial_parameters;
            if (updated_parameters.encodings.empty()) {
                updated_parameters.encodings.emplace_back();
            }
            if (initial_parameters != updated_parameters) {
                channel_->voice_media_send_channel()->SetRtpSendParameters(ssrc_, updated_parameters);
            }

            if (encryptor) {
                channel_->voice_media_send_channel()->SetEncoderToPacketizerFrameTransformer(
                    media_content.ssrc,
                    webrtc::make_ref_counted<FrameTransformer>(
                        true,
                        encryptor,
                        media_content.user_id,
                        payload_type_mapping,
                        get_audio_level_and_speech,
                        nullptr
                    )
                );
            }
        });
    }

    void OutgoingAudioChannel::set_enabled(const bool enable) const {
        channel_->Enable(enable);
        worker_thread_.BlockingCall([&] {
            channel_->voice_media_send_channel()->SetAudioSend(ssrc_, enable, nullptr, sink_);
        });
    }

    OutgoingAudioChannel::~OutgoingAudioChannel() {
        channel_->Enable(false);
        network_thread_.BlockingCall([&] {
            channel_->SetRtpTransport(nullptr);
        });
        worker_thread_.BlockingCall([&] {
            channel_ = nullptr;
        });
        sink_ = nullptr;
    }

    uint32_t OutgoingAudioChannel::ssrc() const {
        return ssrc_;
    }
} // wrtc::interfaces::media::channels