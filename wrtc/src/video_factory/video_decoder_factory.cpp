//
// Created by Lauren on 18/08/23.
//

#include <wrtc/video_factory/video_decoder_factory.hpp>

namespace wrtc::video_factory {
    // TODO: Needed template like this:
    // https://github.com/pytgcalls/ntgcalls/blob/85ee93f72f223405174759b23eb222373e0bc775/wrtc/video_factory/base_video_factory.cpp

    std::unique_ptr<webrtc::VideoDecoder> VideoDecoderFactory::Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat &format) {
        int n = 0;
        for (const auto& enc : decoders_) {
            for (auto supported_formats = formats_[n++]; const auto& f : supported_formats) {
                if (f.IsSameCodec(format)) {
                    return enc.create_video_codec(env, format);
                }
            }
        }
        return nullptr;
    }

    std::vector<webrtc::SdpVideoFormat> VideoDecoderFactory::GetSupportedFormats() const {
        formats_.clear();
        std::vector<webrtc::SdpVideoFormat> r;
        for (const auto& enc : decoders_) {
            auto formats = enc.get_supported_formats();
            r.insert(r.end(), formats.begin(), formats.end());
            formats_.push_back(formats);
        }
        return r;
    }
} // wrtc::video_factory