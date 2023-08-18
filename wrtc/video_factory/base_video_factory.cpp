//
// Created by Laky64 on 18/08/2023.
//


#include "base_video_factory.hpp"

namespace wrtc {

    template <typename Y>
    std::vector<webrtc::SdpVideoFormat> BaseVideoFactory::internalFormats(std::vector<Y> codecs) const {
        static_assert(std::is_same_v<Y, VideoEncoderConfig> || std::is_same_v<Y, VideoDecoderConfig>, "Unsupported type for GetSupportedFormats");

        formats_.clear();
        std::vector<webrtc::SdpVideoFormat> r;
        for (auto& enc : codecs) {
            auto formats = enc.GetSupportedFormats();
            r.insert(r.end(), formats.begin(), formats.end());
            formats_.push_back(formats);
        }
        return r;
    }

    template <class X, class Y>
    std::unique_ptr<X> BaseVideoFactory::internalVideo(std::vector<Y> input, const webrtc::SdpVideoFormat& format) {
        static_assert(std::is_same_v<Y, VideoEncoderConfig> || std::is_same_v<Y, VideoDecoderConfig>, "Unsupported type for CreateVideoCodec");
        static_assert(std::is_same_v<X, webrtc::VideoEncoder> || std::is_same_v<X, webrtc::VideoDecoder>, "Unsupported return type for CreateVideoCodec");
        static_assert((std::is_same_v<Y, VideoEncoderConfig> && std::is_same_v<X, webrtc::VideoEncoder>) ||
              (std::is_same_v<Y, VideoDecoderConfig> && std::is_same_v<X, webrtc::VideoDecoder>), "Mismatch return type and type for CreateVideoCodec");

        int n = 0;
        for (auto& enc : input) {
            auto supported_formats = formats_[n++];

            for (const auto& f : supported_formats) {
                if (f.IsSameCodec(format)) {
                    return enc.CreateVideoCodec(format);
                }
            }
        }
        return nullptr;
    }
} // wrtc