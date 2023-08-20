//
// Created by Laky64 on 18/08/2023.
//

#pragma once


#include <memory>
#include <media/base/media_constants.h>
#include <api/video_codecs/video_encoder.h>
#include <api/video_codecs/video_decoder.h>
#include <api/video_codecs/sdp_video_format.h>

#include <modules/video_coding/codecs/av1/av1_svc_config.h>
#include <modules/video_coding/codecs/h264/include/h264.h>
#include <modules/video_coding/codecs/vp9/include/vp9.h>

namespace wrtc {

    typedef std::function<std::unique_ptr<webrtc::VideoEncoder>(const webrtc::SdpVideoFormat&)> EncoderCallback;
    typedef std::function<std::unique_ptr<webrtc::VideoDecoder>(const webrtc::SdpVideoFormat&)> DecoderCallback;
    typedef std::function<std::vector<webrtc::SdpVideoFormat>()> FormatsRetriever;

    class VideoBaseConfig {
    public:
        virtual ~VideoBaseConfig() = default;

        std::vector<webrtc::SdpVideoFormat> GetSupportedFormats();

    protected:
        webrtc::VideoCodecType codec = webrtc::VideoCodecType::kVideoCodecGeneric;
        FormatsRetriever formatsRetriever;

        virtual bool isInternal() = 0;

        virtual std::vector<webrtc::SdpVideoFormat> getInternalFormats() = 0;

    private:
        std::vector<webrtc::SdpVideoFormat> getDefaultFormats();
    };

} // wrtc
