//
// Created by Laky64 on 10/04/25.
//

#include <rtc_base/logging.h>
#include <wrtc/video_factory/hardware/amd/amd.hpp>
#include <wrtc/video_factory/hardware/amd/amd_decoder.hpp>
#include <wrtc/video_factory/hardware/amd/amd_encoder.hpp>

namespace amd {
    void addEncoders(std::vector<wrtc::VideoEncoderConfig>& encoders) {
        if (AMDEncoder::IsSupported(webrtc::kVideoCodecH264)) {
            RTC_LOG(LS_INFO) << "Adding AMD H264 encoder";
            encoders.insert(
                encoders.begin(),
                {
                    webrtc::kVideoCodecH264,
                    [](const auto&) {
                        return std::make_unique<AMDEncoder>(webrtc::kVideoCodecH264);
                    }
                }
            );
        }

        if (AMDEncoder::IsSupported(webrtc::kVideoCodecH265)) {
            encoders.insert(
                encoders.begin(),
                {
                    webrtc::kVideoCodecH265,
                    [](const auto&) {
                        return std::make_unique<AMDEncoder>(webrtc::kVideoCodecH265);
                    }
                }
            );
        }

        if (AMDEncoder::IsSupported(webrtc::kVideoCodecAV1)) {
            encoders.insert(
                encoders.begin(),
                {
                    webrtc::kVideoCodecAV1,
                    [](const auto&) {
                        return std::make_unique<AMDEncoder>(webrtc::kVideoCodecAV1);
                    }
                }
            );
        }
    }

    void addDecoders(std::vector<wrtc::VideoDecoderConfig>& decoders) {
        if (AMDDecoder::IsSupported(webrtc::kVideoCodecH264)) {
            decoders.insert(
                decoders.begin(),
                {
                    webrtc::kVideoCodecH264,
                    [](const auto&) {
                        return std::make_unique<AMDDecoder>(webrtc::kVideoCodecH264);
                    }
                }
            );
        }

        if (AMDDecoder::IsSupported(webrtc::kVideoCodecH265)) {
            decoders.insert(
                decoders.begin(),
                {
                    webrtc::kVideoCodecH265,
                    [](const auto&) {
                        return std::make_unique<AMDDecoder>(webrtc::kVideoCodecH265);
                    }
                }
            );
        }

        if (AMDDecoder::IsSupported(webrtc::kVideoCodecAV1)) {
            decoders.insert(
                decoders.begin(),
                {
                    webrtc::kVideoCodecAV1,
                    [](const auto&) {
                        return std::make_unique<AMDDecoder>(webrtc::kVideoCodecAV1);
                    }
                }
            );
        }
    }
} // amd