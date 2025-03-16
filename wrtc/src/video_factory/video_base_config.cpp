//
// Created by Laky64 on 18/08/2023.
//

#include <wrtc/video_factory/video_base_config.hpp>

#include <media/base/media_constants.h>
#include <modules/video_coding/codecs/av1/av1_svc_config.h>
#include <modules/video_coding/codecs/h264/include/h264.h>
#include <modules/video_coding/codecs/vp9/include/vp9.h>

namespace wrtc {

    std::vector<webrtc::SdpVideoFormat> VideoBaseConfig::GetSupportedFormats() {
        if (isInternal()) {
            return getInternalFormats();
        }
        if (formatsRetriever) {
            return formatsRetriever();
        }
        return getDefaultFormats();
    }

    std::vector<webrtc::SdpVideoFormat> VideoBaseConfig::getDefaultFormats() const
    {
        std::vector<webrtc::SdpVideoFormat> r;
        if (codec == webrtc::kVideoCodecVP8) {
            r.emplace_back(cricket::kVp8CodecName);
        } else if (codec == webrtc::kVideoCodecVP9) {
            for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs(true)) {
                r.push_back(format);
            }
        } else if (codec == webrtc::kVideoCodecAV1) {
            r.emplace_back(
                cricket::kAv1CodecName,
                webrtc::CodecParameterMap(),
                webrtc::LibaomAv1EncoderSupportedScalabilityModes()
            );
        } else if (codec == webrtc::kVideoCodecH264) {
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
        } else if (codec == webrtc::kVideoCodecH265) {
            r.emplace_back(cricket::kH265CodecName);
        }
        return r;
    }
} // wrtc