//
// Created by Laky64 on 18/08/2023.
//


#include "video_coding.hpp"

namespace wrtc {

    VideoCoding VideoDecoder::CreateVideoCoding(const webrtc::SdpVideoFormat& format) {
        VideoCoding res;
        auto config = GetVideoFactoryConfig();
        res.encoder = CreateBaseVideo<webrtc::VideoEncoder>(config->encoders, format);
        res.decoder = CreateBaseVideo<webrtc::VideoDecoder>(config->decoders, format);
        return res;
    }

    template <typename T>
    std::vector<std::vector<webrtc::SdpVideoFormat>> VideoDecoder::GetSupportedFormats(std::vector<T> codecs) const {
        static_assert(std::is_same_v<T, VideoEncoderConfig> || std::is_same_v<T, VideoDecoderConfig>, "Unsupported type for GetSupportedFormats");
        std::vector<std::vector<webrtc::SdpVideoFormat>> r;
        for (auto& enc : codecs) {
            auto formats = enc.GetSupportedFormats();
            r.push_back(formats);
        }
        return r;
    }

    template <typename X, typename T>
    std::unique_ptr<X> VideoDecoder::CreateBaseVideo(std::vector<T> input, const webrtc::SdpVideoFormat& format) {
        static_assert(std::is_same_v<T, VideoEncoderConfig> || std::is_same_v<T, VideoDecoderConfig>, "Unsupported type for CreateVideoCodec");
        static_assert(std::is_same_v<X, webrtc::VideoEncoder> || std::is_same_v<X, webrtc::VideoDecoder>, "Unsupported return type for CreateVideoCodec");
        static_assert((std::is_same_v<T, VideoEncoderConfig> && std::is_same_v<X, webrtc::VideoEncoder>) ||
              (std::is_same_v<T, VideoDecoderConfig> && std::is_same_v<X, webrtc::VideoDecoder>), "Mismatch return type and type for CreateVideoCodec");

        auto formats = GetSupportedFormats(input);
        int n = 0;
        for (auto& enc : input) {
            auto supported_formats = formats[n++];

            for (const auto& f : supported_formats) {
                if (f.IsSameCodec(format)) {
                    return enc.CreateVideoCodec(format);
                }
            }
        }
        return nullptr;
    }


    std::shared_ptr<VideoFactoryConfig> VideoDecoder::GetVideoFactoryConfig() {
        std::shared_ptr<VideoFactoryConfig> config;

        // Google (Software, VP9, VP8)
        google::addEncoders(config);
        google::addDecoders(config);

        // VLC (Software, AV1)
        vlc::addEncoders(config);
        vlc::addDecoders(config);

        // NVCODEC (Hardware, VP8, VP9, H264)
        // TODO: @Laky-64 Add NVCODEC encoder-decoder when available

        return config;
    }
} // wrtc