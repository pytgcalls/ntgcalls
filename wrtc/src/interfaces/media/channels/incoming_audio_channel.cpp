//
// Created by Lauren on 03/10/24.
//

#include <functional>
#include <rtc_base/time_utils.h>
#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/media/raw_audio_sink.hpp>
#include <wrtc/interfaces/media/channels/incoming_audio_channel.hpp>

namespace wrtc::interfaces::media::channels {
    IncomingAudioChannel::IncomingAudioChannel(
        webrtc::Call* call,
        ChannelManager* channel_manager,
        webrtc::RtpTransport* rtp_transport,
        const models::MediaContent& media_content,
        utils::SafeThread& worker_thread,
        utils::SafeThread& network_thread,
        std::weak_ptr<RemoteAudioSink> remote_audio_sink,
        const std::map<int32_t, FrameTransformer::PayloadType>& payload_type_mapping,
        E2EEncryptor* encryptor,
        const std::function<void(uint32_t, uint8_t, bool)>& set_audio_level_and_speech
    ): ssrc_(media_content.ssrc), worker_thread_(worker_thread), network_thread_(network_thread) {
        update_activity();

        const auto stream_id = std::to_string(ssrc_);

        webrtc::AudioOptions audio_options;
        audio_options.audio_jitter_buffer_fast_accelerate = true;
        audio_options.audio_jitter_buffer_min_delay_ms = 50;

        channel_ = channel_manager->create_voice_channel(
            call,
            webrtc::MediaConfig(),
            stream_id,
            false,
            NativeNetworkInterface::get_default_crypto_options(),
            audio_options
        );
        network_thread.BlockingCall([&] {
            channel_->SetRtpTransport(rtp_transport);
        });
        std::vector<webrtc::Codec> codecs;
        for (const auto& [id, name, clockrate, channels, feedbackTypes, parameters] : media_content.payload_types) {
            webrtc::Codec codec = webrtc::CreateAudioCodec(id, name, clockrate, channels);
            codec.SetParam(webrtc::kCodecParamUseInbandFec, 1);
            codec.SetParam(webrtc::kCodecParamPTime, 60);
            for (const auto& [type, subtype] : feedbackTypes) {
                codec.AddFeedbackParam(webrtc::FeedbackParam(type, subtype));
            }
            codecs.push_back(std::move(codec));
            break;
        }

        const auto outgoing_description = std::make_unique<webrtc::AudioContentDescription>();
        for (const auto& rtp_extension : media_content.rtp_extensions) {
            outgoing_description->AddRtpHeaderExtension(webrtc::RtpExtension(rtp_extension.uri, rtp_extension.id));
        }
        outgoing_description->set_rtcp_mux(true);
        outgoing_description->set_rtcp_reduced_size(true);
        outgoing_description->set_direction(webrtc::RtpTransceiverDirection::kRecvOnly);
        outgoing_description->set_codecs(codecs);
        outgoing_description->set_bandwidth(-1);

        const auto incoming_description = std::make_unique<webrtc::AudioContentDescription>();
        for (const auto& rtp_extension : media_content.rtp_extensions) {
            incoming_description->AddRtpHeaderExtension(webrtc::RtpExtension(rtp_extension.uri, rtp_extension.id));
        }
        incoming_description->set_rtcp_mux(true);
        incoming_description->set_rtcp_reduced_size(true);
        incoming_description->set_direction(webrtc::RtpTransceiverDirection::kSendOnly);
        incoming_description->set_codecs(codecs);
        incoming_description->set_bandwidth(-1);

        webrtc::StreamParams stream_params = webrtc::StreamParams::CreateLegacy(media_content.ssrc);
        stream_params.set_stream_ids({stream_id});
        incoming_description->AddStream(stream_params);

        worker_thread.BlockingCall([&] {
            channel_->rtp_transport()->SetActivePayloadTypeDemuxing(true);
            channel_->SetLocalContent(outgoing_description.get(), webrtc::SdpType::kOffer);
            channel_->SetRemoteContent(incoming_description.get(), webrtc::SdpType::kAnswer);
        });
        channel_->Enable(true);
        std::function<void(uint8_t, bool)> audio_level_and_speech;
        if (set_audio_level_and_speech) {
            audio_level_and_speech = [media_content, set_audio_level_and_speech](const uint8_t audio_level, const bool has_speech) {
                set_audio_level_and_speech(media_content.ssrc, audio_level, has_speech);
            };
        }
        worker_thread.BlockingCall([&] {
            auto raw_sink = std::make_unique<RawAudioSink>();
            raw_sink->set_remote_audio_sink(ssrc_, [remote_audio_sink](std::unique_ptr<models::AudioFrame> frame) {
                if (const auto remote_audio = remote_audio_sink.lock()) {
                    remote_audio->send_data(std::move(frame));
                }
            });

            if (encryptor) {
                channel_->voice_media_receive_channel()->SetDepacketizerToDecoderFrameTransformer(
                    media_content.ssrc,
                    webrtc::make_ref_counted<FrameTransformer>(
                        false,
                        encryptor,
                        media_content.user_id,
                        payload_type_mapping,
                        nullptr,
                        audio_level_and_speech
                    )
                );
            }

            channel_->voice_media_receive_channel()->SetRawAudioSink(ssrc_, std::move(raw_sink));
        });
    }

    IncomingAudioChannel::~IncomingAudioChannel() {
        channel_->Enable(false);
        network_thread_.BlockingCall([&] {
            channel_->SetRtpTransport(nullptr);
        });
        worker_thread_.BlockingCall([&] {
            channel_->voice_media_receive_channel()->SetDepacketizerToDecoderFrameTransformer(ssrc_, nullptr);
            channel_->voice_media_receive_channel()->SetRawAudioSink(ssrc_, nullptr);
            channel_ = nullptr;
        });
    }

    void IncomingAudioChannel::update_activity() {
        activity_timestamp_ = webrtc::TimeMillis();
    }

    int64_t IncomingAudioChannel::get_activity() const {
        return activity_timestamp_;
    }

    uint32_t IncomingAudioChannel::ssrc() const {
        return ssrc_;
    }
} // wrtc::interfaces::media::channels
