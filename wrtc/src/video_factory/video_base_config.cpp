//
// Created by Lauren on 18/08/23.
//

#include <wrtc/video_factory/video_base_config.hpp>

#include <media/base/media_constants.h>
#include <modules/video_coding/codecs/av1/av1_svc_config.h>
#include <modules/video_coding/codecs/h264/include/h264.h>
#include <modules/video_coding/codecs/vp9/include/vp9.h>

namespace wrtc::video_factory {

    std::vector<webrtc::SdpVideoFormat> VideoBaseConfig::get_supported_formats() const {
        if (is_internal()) {
            return get_internal_formats();
        }
        if (formats_retriever_) {
            return formats_retriever_();
        }
        return get_default_formats();
    }

    std::vector<webrtc::SdpVideoFormat> VideoBaseConfig::get_default_formats() const {
        std::vector<webrtc::SdpVideoFormat> r;
        if (codec_ == webrtc::kVideoCodecVP8) {
            r.emplace_back(webrtc::kVp8CodecName);
        } else if (codec_ == webrtc::kVideoCodecVP9) {
            for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs(true)) {
                r.push_back(format);
            }
        } else if (codec_ == webrtc::kVideoCodecAV1) {
            r.emplace_back(
                webrtc::kAv1CodecName,
                webrtc::CodecParameterMap(),
                webrtc::LibaomAv1EncoderSupportedScalabilityModes()
            );
        } else if (codec_ == webrtc::kVideoCodecH264) {
            r.push_back(
                CreateH264Format(
                    webrtc::H264Profile::kProfileBaseline,
                    webrtc::H264Level::kLevel3_1,
                    "1",
                    true
                )
            );
            r.push_back(
                CreateH264Format(
                    webrtc::H264Profile::kProfileBaseline,
                    webrtc::H264Level::kLevel3_1,
                    "0",
                    true
                )
            );
            r.push_back(
                CreateH264Format(
                    webrtc::H264Profile::kProfileConstrainedBaseline,
                    webrtc::H264Level::kLevel3_1,
                    "1",
                    true
                )
            );
            r.push_back(
                CreateH264Format(
                    webrtc::H264Profile::kProfileConstrainedBaseline,
                    webrtc::H264Level::kLevel3_1,
                    "0",
                    true
                )
            );
        } else if (codec_ == webrtc::kVideoCodecH265) {
            r.emplace_back(webrtc::kH265CodecName);
        }
        return r;
    }
} // wrtc::video_factory
