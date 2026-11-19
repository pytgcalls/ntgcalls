//
// Created by Lauren on 20/11/26.
//

#include <api/video/encoded_image.h>
#include <media/base/media_constants.h>
#include <modules/video_coding/include/video_error_codes.h>
#include <wrtc/video_factory/passthrough/passthrough_video_decoder.hpp>

namespace wrtc::video_factory {
    PassthroughVideoDecoder::PassthroughVideoDecoder(std::shared_ptr<utils::encoded::EncodedQueue> queue, const std::string& codec_name): queue_(std::move(queue)) {
        if (codec_name == webrtc::kVp8CodecName) {
            codec_ = models::Codec::VP8;
        } else if (codec_name == webrtc::kVp9CodecName) {
            codec_ = models::Codec::VP9;
        } else if (codec_name == webrtc::kAv1CodecName) {
            codec_ = models::Codec::AV1;
        } else {
            codec_ = models::Codec::H264;
        }
        placeholder_ = webrtc::I420Buffer::Create(2, 2);
        webrtc::I420Buffer::SetBlack(placeholder_.get());
    }

    bool PassthroughVideoDecoder::Configure(const Settings&) {
        return true;
    }

    int32_t PassthroughVideoDecoder::Decode(const webrtc::EncodedImage& input_image, const int64_t render_time_ms) {
        if (!callback_) {
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        if (input_image._encodedWidth > 0 && input_image._encodedHeight > 0) {
            width_ = static_cast<uint16_t>(input_image._encodedWidth);
            height_ = static_cast<uint16_t>(input_image._encodedHeight);
        }
        const bool keyframe = input_image._frameType == webrtc::VideoFrameType::kVideoFrameKey;
        queue_->push({{input_image.data(), input_image.data() + input_image.size()}, {render_time_ms, input_image.rotation_, width_, height_, codec_, keyframe}});
        auto frame = webrtc::VideoFrame::Builder()
                         .set_video_frame_buffer(placeholder_)
                         .set_rtp_timestamp(input_image.RtpTimestamp())
                         .set_timestamp_ms(render_time_ms)
                         .set_rotation(input_image.rotation_)
                         .build();
        callback_->Decoded(frame);
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t PassthroughVideoDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) {
        callback_ = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t PassthroughVideoDecoder::Release() {
        callback_ = nullptr;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    const char* PassthroughVideoDecoder::ImplementationName() const {
        return "PassthroughVideoDecoder";
    }
} // wrtc::video_factory
