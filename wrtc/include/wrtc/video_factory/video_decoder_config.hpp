//
// Created by Lauren on 18/08/23.
//

#pragma once

#include <api/video_codecs/video_decoder_factory.h>
#include <wrtc/video_factory/video_base_config.hpp>

namespace wrtc::video_factory {

    class VideoDecoderConfig final: public VideoBaseConfig {
    public:
        VideoDecoderConfig() = default;

        ~VideoDecoderConfig() override;

        VideoDecoderConfig(webrtc::VideoCodecType codec, DecoderCallback create_video_decoder);

        VideoDecoderConfig(FormatsRetriever get_supported_formats, DecoderCallback create_video_decoder);

        explicit VideoDecoderConfig(std::unique_ptr<webrtc::VideoDecoderFactory> factory): factory_(std::move(factory)) {}

        [[nodiscard]] std::unique_ptr<webrtc::VideoDecoder> create_video_codec(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) const;

    private:
        DecoderCallback decoder_;
        std::shared_ptr<webrtc::VideoDecoderFactory> factory_;

    protected:
        bool is_internal() const override;

        std::vector<webrtc::SdpVideoFormat> get_internal_formats() const override;
    };

} // wrtc::video_factory
