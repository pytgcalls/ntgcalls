//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include <functional>

#include <api/video_codecs/video_encoder.h>
#include <api/video_codecs/video_encoder_factory.h>

#include <wrtc/video_factory/video_base_config.hpp>

namespace wrtc {

    class VideoEncoderConfig final : public VideoBaseConfig {
    public:
        VideoEncoderConfig() = default;

        ~VideoEncoderConfig() override;

        VideoEncoderConfig(webrtc::VideoCodecType codec, EncoderCallback encoder, int alignment = 0);

        VideoEncoderConfig(FormatsRetriever formatsRetriever, EncoderCallback encoder, int alignment = 0);

        explicit VideoEncoderConfig(std::unique_ptr<webrtc::VideoEncoderFactory> factory): factory(std::move(factory)) {}

        [[nodiscard]] std::unique_ptr<webrtc::VideoEncoder> CreateVideoCodec(const webrtc::Environment& env, const webrtc::SdpVideoFormat &format) const;

    private:
        EncoderCallback encoder;
        std::shared_ptr<webrtc::VideoEncoderFactory> factory;
        int alignment = 0;

        bool isInternal() override;

        std::vector<webrtc::SdpVideoFormat> getInternalFormats() override;
    };

} // wrtc
