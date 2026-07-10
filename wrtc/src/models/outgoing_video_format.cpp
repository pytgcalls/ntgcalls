//
// Created by Lauren on 04/11/24.
//

#include <absl/strings/match.h>
#include <rtc_base/logging.h>
#include <wrtc/models/outgoing_video_format.hpp>

namespace wrtc::models {
    OutgoingVideoFormat::OutgoingVideoFormat(webrtc::Codec video_codec, std::optional<webrtc::Codec> rtx_codec) :
    video_codec_(std::move(video_codec)), rtx_codec_(std::move(rtx_codec)){}

    std::vector<webrtc::Codec> OutgoingVideoFormat::get_video_codecs(
        const std::vector<webrtc::SdpVideoFormat>& formats,
        const std::vector<PayloadType>& payload_types,
        const bool is_group_connection
    ) {
        std::vector<webrtc::Codec> codecs;
        if (is_group_connection) {
            for (const auto assigned_payloads = assign_payload_types(formats); const auto &payload_type : assigned_payloads) {
                codecs.push_back(payload_type.video_codec_);
                if (payload_type.rtx_codec_) {
                    codecs.push_back(payload_type.rtx_codec_.value());
                }
            }
        } else {
            for (const auto &payload_type : payload_types) {
                webrtc::Codec codec = webrtc::CreateVideoCodec(payload_type.id, payload_type.name);
                for (const auto & [fst, snd] : payload_type.parameters) {
                    codec.SetParam(fst, snd);
                }
                for (const auto & [type, subtype] : payload_type.feedback_types) {
                    codec.AddFeedbackParam(webrtc::FeedbackParam(type, subtype));
                }
                codecs.push_back(std::move(codec));
            }
        }
        return codecs;
    }

    webrtc::Codec OutgoingVideoFormat::video_codec() const {
        return video_codec_;
    }

    std::vector<OutgoingVideoFormat> OutgoingVideoFormat::assign_payload_types(std::vector<webrtc::SdpVideoFormat> const& formats) {
        if (formats.empty()) {
            return {};
        }

        constexpr int kFirstDynamicPayloadType = 100;

        int payload_type = kFirstDynamicPayloadType;

        std::vector<OutgoingVideoFormat> result;

        const std::vector<std::string> filter_codec_names = {
            webrtc::kVp8CodecName,
            webrtc::kVp9CodecName,
            webrtc::kH264CodecName,
        };

        for (const auto &codec_name : filter_codec_names) {
            for (const auto &format : formats) {
                constexpr int kLastDynamicPayloadType = 127;
                if (format.name != codec_name) {
                    continue;
                }

                webrtc::Codec codec = webrtc::CreateVideoCodec(format);
                codec.id = payload_type;
                add_default_feedback_params(&codec);

                ++payload_type;
                if (payload_type > kLastDynamicPayloadType) {
                    RTC_LOG(LS_ERROR) << "Out of dynamic payload types, skipping the rest.";
                    break;
                }

                std::optional<webrtc::Codec> rtx_codec;
                if (!absl::EqualsIgnoreCase(codec.name, webrtc::kUlpfecCodecName) && !absl::EqualsIgnoreCase(codec.name, webrtc::kFlexfecCodecName)) {
                    rtx_codec = webrtc::CreateVideoRtxCodec(payload_type, codec.id);

                    ++payload_type;
                    if (payload_type > kLastDynamicPayloadType) {
                        RTC_LOG(LS_ERROR) << "Out of dynamic payload types, skipping the rest.";
                        break;
                    }
                }

                OutgoingVideoFormat result_format(codec, rtx_codec);
                result.push_back(std::move(result_format));
            }
        }

        return result;
    }

    void OutgoingVideoFormat::add_default_feedback_params(webrtc::Codec* codec) {
        if (codec->name == webrtc::kRedCodecName || codec->name == webrtc::kUlpfecCodecName) {
            return;
        }
        codec->AddFeedbackParam(webrtc::FeedbackParam(webrtc::kRtcpFbParamRemb, webrtc::kParamValueEmpty));
        codec->AddFeedbackParam(webrtc::FeedbackParam(webrtc::kRtcpFbParamTransportCc, webrtc::kParamValueEmpty));
        if (codec->name == webrtc::kFlexfecCodecName) {
            return;
        }
        codec->AddFeedbackParam(webrtc::FeedbackParam(webrtc::kRtcpFbParamCcm, webrtc::kRtcpFbCcmParamFir));
        codec->AddFeedbackParam(webrtc::FeedbackParam(webrtc::kRtcpFbParamNack, webrtc::kParamValueEmpty));
        codec->AddFeedbackParam(webrtc::FeedbackParam(webrtc::kRtcpFbParamNack, webrtc::kRtcpFbNackParamPli));
    }
} // wrtc::models