//
// Created by Lauren on 18/08/23.
//

#pragma once


#include <memory>
#include <api/video_codecs/sdp_video_format.h>
#include <api/video_codecs/video_decoder.h>
#include <api/video_codecs/video_encoder.h>

namespace wrtc::video_factory {

    typedef std::function<std::unique_ptr<webrtc::VideoEncoder>(const webrtc::SdpVideoFormat&)> EncoderCallback;
    typedef std::function<std::unique_ptr<webrtc::VideoDecoder>(const webrtc::SdpVideoFormat&)> DecoderCallback;
    typedef std::function<std::vector<webrtc::SdpVideoFormat>()> FormatsRetriever;

    class VideoBaseConfig {
    public:
        virtual ~VideoBaseConfig() = default;

        std::vector<webrtc::SdpVideoFormat> get_supported_formats() const;

    protected:
        webrtc::VideoCodecType codec_ = webrtc::VideoCodecType::kVideoCodecGeneric;
        FormatsRetriever formats_retriever_;

        virtual bool is_internal() const = 0;

        virtual std::vector<webrtc::SdpVideoFormat> get_internal_formats() const = 0;

    private:
        [[nodiscard]] std::vector<webrtc::SdpVideoFormat> get_default_formats() const;
    };

} // wrtc::video_factory
