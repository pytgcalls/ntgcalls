//
// Created by Laky64 on 04/11/24.
//

#include <absl/strings/match.h>
#include <rtc_base/logging.h>
#include <wrtc/models/outgoing_video_format.hpp>

namespace wrtc {
    OutgoingVideoFormat::OutgoingVideoFormat(cricket::Codec videoCodec_, std::optional<cricket::Codec> rtxCodec_) :
    videoCodec(std::move(videoCodec_)), rtxCodec(std::move(rtxCodec_)){}

    std::vector<cricket::Codec> OutgoingVideoFormat::getVideoCodecs(
        const std::vector<webrtc::SdpVideoFormat>& formats,
        const std::vector<PayloadType>& payloadTypes,
        const bool isGroupConnection
    ) {
        std::vector<cricket::Codec> codecs;
        if (isGroupConnection) {
            for (auto assignedPayloads = assignPayloadTypes(formats); const auto &payloadType : assignedPayloads) {
                codecs.push_back(payloadType.videoCodec);
                if (payloadType.rtxCodec) {
                    codecs.push_back(payloadType.rtxCodec.value());
                }
            }
        } else {
            for (const auto &payloadType : payloadTypes) {
                cricket::Codec codec = cricket::CreateVideoCodec(payloadType.id, payloadType.name);
                for (const auto & [fst, snd] : payloadType.parameters) {
                    codec.SetParam(fst, snd);
                }
                for (const auto & [type, subtype] : payloadType.feedbackTypes) {
                    codec.AddFeedbackParam(cricket::FeedbackParam(type, subtype));
                }
                codecs.push_back(std::move(codec));
            }
        }
        return codecs;
    }

    std::vector<OutgoingVideoFormat> OutgoingVideoFormat::assignPayloadTypes(std::vector<webrtc::SdpVideoFormat> const& formats) {
        if (formats.empty()) {
            return {};
        }

        constexpr int kFirstDynamicPayloadType = 100;

        int payload_type = kFirstDynamicPayloadType;

        std::vector<OutgoingVideoFormat> result;

        std::vector<std::string> filterCodecNames = {
            cricket::kVp8CodecName,
            cricket::kVp9CodecName,
            cricket::kH264CodecName,
        };

        for (const auto &codecName : filterCodecNames) {
            for (const auto &format : formats) {
                constexpr int kLastDynamicPayloadType = 127;
                if (format.name != codecName) {
                    continue;
                }

                cricket::Codec codec = cricket::CreateVideoCodec(format);
                codec.id = payload_type;
                addDefaultFeedbackParams(&codec);

                ++payload_type;
                if (payload_type > kLastDynamicPayloadType) {
                    RTC_LOG(LS_ERROR) << "Out of dynamic payload types, skipping the rest.";
                    break;
                }

                std::optional<cricket::Codec> rtxCodec;
                if (!absl::EqualsIgnoreCase(codec.name, cricket::kUlpfecCodecName) && !absl::EqualsIgnoreCase(codec.name, cricket::kFlexfecCodecName)) {
                    rtxCodec = cricket::CreateVideoRtxCodec(payload_type, codec.id);

                    ++payload_type;
                    if (payload_type > kLastDynamicPayloadType) {
                        RTC_LOG(LS_ERROR) << "Out of dynamic payload types, skipping the rest.";
                        break;
                    }
                }

                OutgoingVideoFormat resultFormat(codec, rtxCodec);

                result.push_back(std::move(resultFormat));
            }
        }

        return result;
    }

    void OutgoingVideoFormat::addDefaultFeedbackParams(cricket::Codec* codec) {
        if (codec->name == cricket::kRedCodecName || codec->name == cricket::kUlpfecCodecName) {
            return;
        }
        codec->AddFeedbackParam(cricket::FeedbackParam(cricket::kRtcpFbParamRemb, cricket::kParamValueEmpty));
        codec->AddFeedbackParam(cricket::FeedbackParam(cricket::kRtcpFbParamTransportCc, cricket::kParamValueEmpty));
        if (codec->name == cricket::kFlexfecCodecName) {
            return;
        }
        codec->AddFeedbackParam(cricket::FeedbackParam(cricket::kRtcpFbParamCcm, cricket::kRtcpFbCcmParamFir));
        codec->AddFeedbackParam(cricket::FeedbackParam(cricket::kRtcpFbParamNack, cricket::kParamValueEmpty));
        codec->AddFeedbackParam(cricket::FeedbackParam(cricket::kRtcpFbParamNack, cricket::kRtcpFbNackParamPli));
    }
} // wrtc