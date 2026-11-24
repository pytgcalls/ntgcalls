//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <api/video/i420_buffer.h>
#include <api/video_codecs/video_decoder.h>
#include <wrtc/utils/encoded/encoded_queue.hpp>

namespace wrtc::video_factory {
    class PassthroughVideoDecoder final: public webrtc::VideoDecoder {
        std::shared_ptr<utils::encoded::EncodedQueue> queue_;
        models::Codec codec_;
        webrtc::DecodedImageCallback* callback_ = nullptr;
        webrtc::scoped_refptr<webrtc::I420Buffer> placeholder_;
        uint16_t width_ = 0;
        uint16_t height_ = 0;

    public:
        PassthroughVideoDecoder(std::shared_ptr<utils::encoded::EncodedQueue> queue, const std::string& codec_name);

        bool Configure(const Settings& settings) override;

        int32_t Decode(const webrtc::EncodedImage& input_image, int64_t render_time_ms) override;

        int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) override;

        int32_t Release() override;

        [[nodiscard]] const char* ImplementationName() const override;
    };
} // wrtc::video_factory
