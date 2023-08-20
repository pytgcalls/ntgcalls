//
// Created by Laky64 on 18/08/2023.
//

#include "video_base_config.hpp"

namespace wrtc {

    std::vector<webrtc::SdpVideoFormat> VideoBaseConfig::GetSupportedFormats() {
        if (isInternal()) {
            return getInternalFormats();
        } else if (formatsRetriever) {
            return formatsRetriever();
        } else {
            return getDefaultFormats();
        }
    }

    std::vector<webrtc::SdpVideoFormat> VideoBaseConfig::getDefaultFormats() {
        std::vector<webrtc::SdpVideoFormat> r;
        if (codec == webrtc::kVideoCodecVP8) {
            r.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
        } else if (codec == webrtc::kVideoCodecVP9) {
            for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs(true)) {
                r.push_back(format);
            }
        } else if (codec == webrtc::kVideoCodecAV1) {
            r.push_back(
                    webrtc::SdpVideoFormat(
                            cricket::kAv1CodecName,
                            webrtc::SdpVideoFormat::Parameters(),
                            webrtc::LibaomAv1EncoderSupportedScalabilityModes()
                    )
            );
        } else if (codec == webrtc::kVideoCodecH264) {
            r.push_back(
                    CreateH264Format(
                            webrtc::H264Profile::kProfileBaseline,
                            webrtc::H264Level::kLevel3_1,
                            "1"
                    )
            );
            r.push_back(
                    CreateH264Format(
                            webrtc::H264Profile::kProfileBaseline,
                            webrtc::H264Level::kLevel3_1,
                            "0"
                    )
            );
            r.push_back(
                    CreateH264Format(
                            webrtc::H264Profile::kProfileConstrainedBaseline,
                            webrtc::H264Level::kLevel3_1,
                            "1"
                    )
            );
            r.push_back(
                    CreateH264Format(
                            webrtc::H264Profile::kProfileConstrainedBaseline,
                            webrtc::H264Level::kLevel3_1,
                            "0"
                    )
            );
        }
        return r;
    }
} // wrtc