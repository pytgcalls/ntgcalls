//
// Created by Lauren on 18/08/23.
//

#pragma once

#include <functional>
#include <api/video_codecs/video_encoder.h>
#include <api/video_codecs/video_encoder_factory.h>
#include <wrtc/video_factory/video_base_config.hpp>

namespace wrtc::video_factory {

    class VideoEncoderConfig final : public VideoBaseConfig {
    public:
        VideoEncoderConfig() = default;

        ~VideoEncoderConfig() override;

        VideoEncoderConfig(webrtc::VideoCodecType codec, EncoderCallback encoder, int alignment = 0);

        VideoEncoderConfig(FormatsRetriever formats_retriever, EncoderCallback encoder, int alignment = 0);

        explicit VideoEncoderConfig(std::unique_ptr<webrtc::VideoEncoderFactory> factory): factory_(std::move(factory)) {}

        [[nodiscard]] std::unique_ptr<webrtc::VideoEncoder> create_video_codec(const webrtc::Environment& env, const webrtc::SdpVideoFormat &format) const;

    private:
        EncoderCallback encoder_;
        std::shared_ptr<webrtc::VideoEncoderFactory> factory_;
        int alignment_ = 0;

    protected:
        bool is_internal() const override;

        std::vector<webrtc::SdpVideoFormat> get_internal_formats() const override;
    };

} // wrtc::video_factory
