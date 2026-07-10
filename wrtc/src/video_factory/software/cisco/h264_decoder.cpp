//
// Created by Lauren on 05/11/24.
//

#ifndef IS_ANDROID
// ReSharper disable CppDFAUnusedValue
#include <common_video/include/video_frame_buffer.h>
#include <modules/video_coding/include/video_error_codes.h>
#include <rtc_base/logging.h>
#include <system_wrappers/include/metrics.h>
#include <wrtc/video_factory/software/openh264/h264_color_space.hpp>
#include <wrtc/video_factory/software/openh264/h264_decoder.hpp>

extern "C" {
    #include <libavutil/imgutils.h>
}

// Currently updated with the latest commit from:
// https://webrtc.googlesource.com/src/+/8037fc6ffa131805248c2a63c3edec69155b05cf

constexpr std::array kPixelFormatsSupported = {
    AV_PIX_FMT_YUV420P,     AV_PIX_FMT_YUV422P,     AV_PIX_FMT_YUV444P,
    AV_PIX_FMT_YUVJ420P,    AV_PIX_FMT_YUVJ422P,    AV_PIX_FMT_YUVJ444P,
    AV_PIX_FMT_YUV420P10LE, AV_PIX_FMT_YUV422P10LE, AV_PIX_FMT_YUV444P10LE
};

constexpr size_t kYPlaneIndex = 0;
constexpr size_t kUPlaneIndex = 1;
constexpr size_t kVPlaneIndex = 2;

namespace openh264 {
    H264Decoder::H264Decoder()
        : ffmpeg_buffer_pool_(true),
          decoded_image_callback_(nullptr),
          has_reported_init_(false),
          has_reported_error_(false) {}

    H264Decoder::~H264Decoder() {
        Release();
    }

    bool H264Decoder::Configure(const Settings& settings) {
        report_init();
        if (settings.codec_type() != webrtc::kVideoCodecH264) {
            report_error();
            return false;
        }
        if (const int32_t ret = Release(); ret != WEBRTC_VIDEO_CODEC_OK) {
            report_error();
            return false;
        }
        RTC_DCHECK(!av_context_);
        av_context_.reset(avcodec_alloc_context3(nullptr));
        av_context_->codec_type = AVMEDIA_TYPE_VIDEO;
        av_context_->codec_id = AV_CODEC_ID_H264;
        if (const webrtc::RenderResolution& resolution = settings.max_render_resolution(); resolution.Valid()) {
            av_context_->coded_width = resolution.Width();
            av_context_->coded_height = resolution.Height();
        }
        av_context_->extradata = nullptr;
        av_context_->extradata_size = 0;

        av_context_->thread_count = 1;
        av_context_->thread_type = FF_THREAD_SLICE;

        av_context_->get_buffer2 = av_get_buffer2;

        av_context_->opaque = this;

        const AVCodec* codec = avcodec_find_decoder(av_context_->codec_id);
        if (!codec) {
            RTC_LOG(LS_ERROR) << "FFmpeg H.264 decoder not found.";
            Release();
            report_error();
            return false;
        }
        if (const int res = avcodec_open2(av_context_.get(), codec, nullptr); res < 0) {
            RTC_LOG(LS_ERROR) << "avcodec_open2 error: " << res;
            Release();
            report_error();
            return false;
        }

        av_frame_.reset(av_frame_alloc());

        if (const std::optional<int> buffer_pool_size = settings.buffer_pool_size()) {
            if (!ffmpeg_buffer_pool_.Resize(*buffer_pool_size)) {
                return false;
            }
        }
        return true;
    }

    int32_t H264Decoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) {
        decoded_image_callback_ = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t H264Decoder::Release() {
        av_context_.reset();
        av_frame_.reset();
        return WEBRTC_VIDEO_CODEC_OK;
    }

    void H264Decoder::report_init() {
        if (has_reported_init_)
            return;
        RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.H264DecoderImpl.Event", H264DecoderEventInit, H264DecoderEventMax);
        has_reported_init_ = true;
    }

    void H264Decoder::report_error() {
        if (has_reported_error_)
            return;
        RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.H264DecoderImpl.Event", H264DecoderEventError, H264DecoderEventMax);
        has_reported_error_ = true;
    }

    int H264Decoder::av_get_buffer2(AVCodecContext* context, AVFrame* av_frame, int flags) {
        auto* decoder = static_cast<H264Decoder*>(context->opaque);
        RTC_DCHECK(decoder);
        RTC_DCHECK(context->codec->capabilities | AV_CODEC_CAP_DR1);

        const auto pixel_format_supported = std::ranges::find_if(kPixelFormatsSupported, [context](const AVPixelFormat format) {
            return context->pix_fmt == format;
        });

        if (pixel_format_supported == kPixelFormatsSupported.end()) {
            RTC_LOG(LS_ERROR) << "Unsupported pixel format: " << context->pix_fmt;
            decoder->report_error();
            return -1;
        }

        int width = av_frame->width;
        int height = av_frame->height;

        RTC_CHECK_EQ(context->lowres, 0);

        avcodec_align_dimensions(context, &width, &height);

        RTC_CHECK_GE(width, 0);
        RTC_CHECK_GE(height, 0);
        if (const int ret = av_image_check_size(static_cast<unsigned int>(width), static_cast<unsigned int>(height), 0, nullptr); ret < 0) {
            RTC_LOG(LS_ERROR) << "Invalid picture size " << width << "x" << height;
            decoder->report_error();
            return ret;
        }

        webrtc::scoped_refptr<webrtc::PlanarYuvBuffer> frame_buffer;
        webrtc::scoped_refptr<webrtc::I444Buffer> i444_buffer;
        webrtc::scoped_refptr<webrtc::I420Buffer> i420_buffer;
        webrtc::scoped_refptr<webrtc::I422Buffer> i422_buffer;
        webrtc::scoped_refptr<webrtc::I010Buffer> i010_buffer;
        webrtc::scoped_refptr<webrtc::I210Buffer> i210_buffer;
        webrtc::scoped_refptr<webrtc::I410Buffer> i410_buffer;
        int bytes_per_pixel = 1;
        switch (context->pix_fmt) {
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P:
            i420_buffer = decoder->ffmpeg_buffer_pool_.CreateI420Buffer(width, height);
            av_frame->data[kYPlaneIndex] = i420_buffer->MutableDataY();
            av_frame->linesize[kYPlaneIndex] = i420_buffer->StrideY();
            av_frame->data[kUPlaneIndex] = i420_buffer->MutableDataU();
            av_frame->linesize[kUPlaneIndex] = i420_buffer->StrideU();
            av_frame->data[kVPlaneIndex] = i420_buffer->MutableDataV();
            av_frame->linesize[kVPlaneIndex] = i420_buffer->StrideV();
            RTC_DCHECK_EQ(av_frame->extended_data, av_frame->data);
            frame_buffer = i420_buffer;
            break;
        case AV_PIX_FMT_YUV444P:
        case AV_PIX_FMT_YUVJ444P:
            i444_buffer = decoder->ffmpeg_buffer_pool_.CreateI444Buffer(width, height);
            av_frame->data[kYPlaneIndex] = i444_buffer->MutableDataY();
            av_frame->linesize[kYPlaneIndex] = i444_buffer->StrideY();
            av_frame->data[kUPlaneIndex] = i444_buffer->MutableDataU();
            av_frame->linesize[kUPlaneIndex] = i444_buffer->StrideU();
            av_frame->data[kVPlaneIndex] = i444_buffer->MutableDataV();
            av_frame->linesize[kVPlaneIndex] = i444_buffer->StrideV();
            frame_buffer = i444_buffer;
            break;
        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUVJ422P:
            i422_buffer = decoder->ffmpeg_buffer_pool_.CreateI422Buffer(width, height);
            av_frame->data[kYPlaneIndex] = i422_buffer->MutableDataY();
            av_frame->linesize[kYPlaneIndex] = i422_buffer->StrideY();
            av_frame->data[kUPlaneIndex] = i422_buffer->MutableDataU();
            av_frame->linesize[kUPlaneIndex] = i422_buffer->StrideU();
            av_frame->data[kVPlaneIndex] = i422_buffer->MutableDataV();
            av_frame->linesize[kVPlaneIndex] = i422_buffer->StrideV();
            frame_buffer = i422_buffer;
            break;
        case AV_PIX_FMT_YUV420P10LE:
            i010_buffer = decoder->ffmpeg_buffer_pool_.CreateI010Buffer(width, height);
            av_frame->data[kYPlaneIndex] = reinterpret_cast<bytes::byte*>(i010_buffer->MutableDataY());
            av_frame->linesize[kYPlaneIndex] = i010_buffer->StrideY() * 2;
            av_frame->data[kUPlaneIndex] = reinterpret_cast<bytes::byte*>(i010_buffer->MutableDataU());
            av_frame->linesize[kUPlaneIndex] = i010_buffer->StrideU() * 2;
            av_frame->data[kVPlaneIndex] = reinterpret_cast<bytes::byte*>(i010_buffer->MutableDataV());
            av_frame->linesize[kVPlaneIndex] = i010_buffer->StrideV() * 2;
            frame_buffer = i010_buffer;
            bytes_per_pixel = 2;
            break;
        case AV_PIX_FMT_YUV422P10LE:
            i210_buffer = decoder->ffmpeg_buffer_pool_.CreateI210Buffer(width, height);
            av_frame->data[kYPlaneIndex] = reinterpret_cast<bytes::byte*>(i210_buffer->MutableDataY());
            av_frame->linesize[kYPlaneIndex] = i210_buffer->StrideY() * 2;
            av_frame->data[kUPlaneIndex] = reinterpret_cast<bytes::byte*>(i210_buffer->MutableDataU());
            av_frame->linesize[kUPlaneIndex] = i210_buffer->StrideU() * 2;
            av_frame->data[kVPlaneIndex] = reinterpret_cast<bytes::byte*>(i210_buffer->MutableDataV());
            av_frame->linesize[kVPlaneIndex] = i210_buffer->StrideV() * 2;
            frame_buffer = i210_buffer;
            bytes_per_pixel = 2;
            break;
        case AV_PIX_FMT_YUV444P10LE:
            i410_buffer = decoder->ffmpeg_buffer_pool_.CreateI410Buffer(width, height);
            av_frame->data[kYPlaneIndex] = reinterpret_cast<bytes::byte*>(i410_buffer->MutableDataY());
            av_frame->linesize[kYPlaneIndex] = i410_buffer->StrideY() * 2;
            av_frame->data[kUPlaneIndex] = reinterpret_cast<bytes::byte*>(i410_buffer->MutableDataU());
            av_frame->linesize[kUPlaneIndex] = i410_buffer->StrideU() * 2;
            av_frame->data[kVPlaneIndex] = reinterpret_cast<bytes::byte*>(i410_buffer->MutableDataV());
            av_frame->linesize[kVPlaneIndex] = i410_buffer->StrideV() * 2;
            frame_buffer = i410_buffer;
            bytes_per_pixel = 2;
            break;
        default:
            RTC_LOG(LS_ERROR) << "Unsupported buffer type " << context->pix_fmt
                              << ". Check supported supported pixel formats!";
            decoder->report_error();
            return -1;
        }

        const int y_size = width * height * bytes_per_pixel;
        const int uv_size = frame_buffer->ChromaWidth() * frame_buffer->ChromaHeight() * bytes_per_pixel;
        RTC_DCHECK_EQ(av_frame->data[kUPlaneIndex], av_frame->data[kYPlaneIndex] + y_size);
        RTC_DCHECK_EQ(av_frame->data[kVPlaneIndex], av_frame->data[kUPlaneIndex] + uv_size);
        const int total_size = y_size + 2 * uv_size;

        av_frame->format = context->pix_fmt;
        av_frame->buf[0] = av_buffer_create(
            av_frame->data[kYPlaneIndex], total_size, av_free_buffer2,
            std::make_unique<webrtc::VideoFrame>(webrtc::VideoFrame::Builder()
                .set_video_frame_buffer(frame_buffer)
                .set_rotation(webrtc::kVideoRotation_0)
                .set_timestamp_us(0)
                .build())
            .release(),
            0);
        RTC_CHECK(av_frame->buf[0]);
        return 0;
    }

    void H264Decoder::av_free_buffer2(void* opaque, bytes::byte*) {
        const auto* video_frame = static_cast<webrtc::VideoFrame*>(opaque);
        delete video_frame;
    }

    bool H264Decoder::is_initialized() const {
        return av_context_ != nullptr;
    }

    int32_t H264Decoder::Decode(const webrtc::EncodedImage& input_image, bool missing_frames, int64_t render_time_ms) {
        if (!is_initialized()) {
            report_error();
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        if (!decoded_image_callback_) {
            RTC_LOG(LS_WARNING)
                << "Configure() has been called, but a callback function "
                   "has not been set with RegisterDecodeCompleteCallback()";
            report_error();
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }

        if (!input_image.data() || !input_image.size()) {
            report_error();
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }

        ScopedAVPacket packet = make_scoped_av_packet();
        if (!packet) {
            report_error();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        packet->data = const_cast<bytes::byte*>(input_image.data());
        if (input_image.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            report_error();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        packet->size = static_cast<int>(input_image.size());
        int result = avcodec_send_packet(av_context_.get(), packet.get());
        if (result < 0) {
            RTC_LOG(LS_ERROR) << "avcodec_send_packet error: " << result;
            report_error();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        result = avcodec_receive_frame(av_context_.get(), av_frame_.get());
        if (result < 0) {
            RTC_LOG(LS_ERROR) << "avcodec_receive_frame error: " << result;
            report_error();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        h264_bitstream_parser_.ParseBitstream(input_image);
        const std::optional<int> qp = h264_bitstream_parser_.GetLastSliceQp();

        const auto* input_frame = static_cast<webrtc::VideoFrame*>(av_buffer_get_opaque(av_frame_->buf[0]));
        RTC_DCHECK(input_frame);
        const webrtc::scoped_refptr<webrtc::VideoFrameBuffer> frame_buffer = input_frame->video_frame_buffer();

        const webrtc::PlanarYuvBuffer* planar_yuv_buffer = nullptr;
        const webrtc::PlanarYuv8Buffer* planar_yuv8_buffer = nullptr;
        const webrtc::PlanarYuv16BBuffer* planar_yuv16_buffer = nullptr;
        const webrtc::VideoFrameBuffer::Type video_frame_buffer_type = frame_buffer->type();
        
        switch (video_frame_buffer_type) {
        case webrtc::VideoFrameBuffer::Type::kI420:
            planar_yuv_buffer = frame_buffer->GetI420();
            planar_yuv8_buffer = reinterpret_cast<const webrtc::PlanarYuv8Buffer*>(planar_yuv_buffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI444:
            planar_yuv_buffer = frame_buffer->GetI444();
            planar_yuv8_buffer = reinterpret_cast<const webrtc::PlanarYuv8Buffer*>(planar_yuv_buffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI422:
            planar_yuv_buffer = frame_buffer->GetI422();
            planar_yuv8_buffer = reinterpret_cast<const webrtc::PlanarYuv8Buffer*>(planar_yuv_buffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI010:
            planar_yuv_buffer = frame_buffer->GetI010();
            planar_yuv16_buffer = reinterpret_cast<const webrtc::PlanarYuv16BBuffer*>(planar_yuv_buffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI210:
            planar_yuv_buffer = frame_buffer->GetI210();
            planar_yuv16_buffer = reinterpret_cast<const webrtc::PlanarYuv16BBuffer*>(planar_yuv_buffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI410:
            planar_yuv_buffer = frame_buffer->GetI410();
            planar_yuv16_buffer = reinterpret_cast<const webrtc::PlanarYuv16BBuffer*>(planar_yuv_buffer);
            break;
        default:
            RTC_LOG(LS_ERROR) << "frame_buffer type: " << static_cast<int32_t>(video_frame_buffer_type) << " is not supported!";
            report_error();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        RTC_DCHECK_LE(av_frame_->width, planar_yuv_buffer->width());
        RTC_DCHECK_LE(av_frame_->height, planar_yuv_buffer->height());

        switch (video_frame_buffer_type) {
        case webrtc::VideoFrameBuffer::Type::kI420:
        case webrtc::VideoFrameBuffer::Type::kI444:
        case webrtc::VideoFrameBuffer::Type::kI422: {
            RTC_DCHECK_GE(av_frame_->data[kYPlaneIndex], planar_yuv8_buffer->DataY());
            RTC_DCHECK_LE(
                av_frame_->data[kYPlaneIndex] + av_frame_->linesize[kYPlaneIndex] * av_frame_->height,
                planar_yuv8_buffer->DataY() +
                planar_yuv8_buffer->StrideY() * planar_yuv8_buffer->height()
            );
            RTC_DCHECK_GE(av_frame_->data[kUPlaneIndex], planar_yuv8_buffer->DataU());
            RTC_DCHECK_LE(
                av_frame_->data[kUPlaneIndex] + av_frame_->linesize[kUPlaneIndex] * planar_yuv8_buffer->ChromaHeight(),
                planar_yuv8_buffer->DataU() + planar_yuv8_buffer->StrideU() * planar_yuv8_buffer->ChromaHeight()
            );
            RTC_DCHECK_GE(av_frame_->data[kVPlaneIndex], planar_yuv8_buffer->DataV());
            RTC_DCHECK_LE(
                av_frame_->data[kVPlaneIndex] + av_frame_->linesize[kVPlaneIndex] * planar_yuv8_buffer->ChromaHeight(),
                planar_yuv8_buffer->DataV() + planar_yuv8_buffer->StrideV() * planar_yuv8_buffer->ChromaHeight()
            );
            break;
        }
        case webrtc::VideoFrameBuffer::Type::kI010:
        case webrtc::VideoFrameBuffer::Type::kI210:
        case webrtc::VideoFrameBuffer::Type::kI410: {
            RTC_DCHECK_GE(av_frame_->data[kYPlaneIndex], reinterpret_cast<const bytes::byte*>(planar_yuv16_buffer->DataY()));
            RTC_DCHECK_LE(
                av_frame_->data[kYPlaneIndex] + av_frame_->linesize[kYPlaneIndex] * av_frame_->height,
                reinterpret_cast<const bytes::byte*>(planar_yuv16_buffer->DataY()) + planar_yuv16_buffer->StrideY() * 2 * planar_yuv16_buffer->height()
            );
            RTC_DCHECK_GE(av_frame_->data[kUPlaneIndex], reinterpret_cast<const bytes::byte*>(planar_yuv16_buffer->DataU()));
            RTC_DCHECK_LE(
                av_frame_->data[kUPlaneIndex] + av_frame_->linesize[kUPlaneIndex] * planar_yuv16_buffer->ChromaHeight(),
                reinterpret_cast<const bytes::byte*>(planar_yuv16_buffer->DataU()) + planar_yuv16_buffer->StrideU() * 2 * planar_yuv16_buffer->ChromaHeight()
            );
            RTC_DCHECK_GE(av_frame_->data[kVPlaneIndex], reinterpret_cast<const bytes::byte*>(planar_yuv16_buffer->DataV()));
            RTC_DCHECK_LE(
                av_frame_->data[kVPlaneIndex] + av_frame_->linesize[kVPlaneIndex] * planar_yuv16_buffer->ChromaHeight(),
                reinterpret_cast<const bytes::byte*>(planar_yuv16_buffer->DataV()) + planar_yuv16_buffer->StrideV() * 2 * planar_yuv16_buffer->ChromaHeight()
            );
            break;
        }
        default:
            RTC_LOG(LS_ERROR) << "frame_buffer type: " << static_cast<int32_t>(video_frame_buffer_type) << " is not supported!";
            // ReSharper disable once CppDFAUnreachableCode
            report_error();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        webrtc::scoped_refptr<webrtc::VideoFrameBuffer> cropped_buffer;
        switch (video_frame_buffer_type) {
        case webrtc::VideoFrameBuffer::Type::kI420:
            cropped_buffer = webrtc::WrapI420Buffer(
                av_frame_->width, av_frame_->height, av_frame_->data[kYPlaneIndex],
                av_frame_->linesize[kYPlaneIndex], av_frame_->data[kUPlaneIndex],
                av_frame_->linesize[kUPlaneIndex], av_frame_->data[kVPlaneIndex],
                av_frame_->linesize[kVPlaneIndex],
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frame_buffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI444:
            cropped_buffer = webrtc::WrapI444Buffer(
                av_frame_->width, av_frame_->height, av_frame_->data[kYPlaneIndex],
                av_frame_->linesize[kYPlaneIndex], av_frame_->data[kUPlaneIndex],
                av_frame_->linesize[kUPlaneIndex], av_frame_->data[kVPlaneIndex],
                av_frame_->linesize[kVPlaneIndex],
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frame_buffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI422:
            cropped_buffer = webrtc::WrapI422Buffer(
                av_frame_->width, av_frame_->height, av_frame_->data[kYPlaneIndex],
                av_frame_->linesize[kYPlaneIndex], av_frame_->data[kUPlaneIndex],
                av_frame_->linesize[kUPlaneIndex], av_frame_->data[kVPlaneIndex],
                av_frame_->linesize[kVPlaneIndex],
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frame_buffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI010:
            cropped_buffer = webrtc::WrapI010Buffer(
                av_frame_->width, av_frame_->height,
                reinterpret_cast<const uint16_t*>(av_frame_->data[kYPlaneIndex]),
                av_frame_->linesize[kYPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(av_frame_->data[kUPlaneIndex]),
                av_frame_->linesize[kUPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(av_frame_->data[kVPlaneIndex]),
                av_frame_->linesize[kVPlaneIndex] / 2,
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frame_buffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI210:
            cropped_buffer = webrtc::WrapI210Buffer(
                av_frame_->width, av_frame_->height,
                reinterpret_cast<const uint16_t*>(av_frame_->data[kYPlaneIndex]),
                av_frame_->linesize[kYPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(av_frame_->data[kUPlaneIndex]),
                av_frame_->linesize[kUPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(av_frame_->data[kVPlaneIndex]),
                av_frame_->linesize[kVPlaneIndex] / 2,
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frame_buffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI410:
            cropped_buffer = webrtc::WrapI410Buffer(
                av_frame_->width, av_frame_->height,
                reinterpret_cast<const uint16_t*>(av_frame_->data[kYPlaneIndex]),
                av_frame_->linesize[kYPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(av_frame_->data[kUPlaneIndex]),
                av_frame_->linesize[kUPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(av_frame_->data[kVPlaneIndex]),
                av_frame_->linesize[kVPlaneIndex] / 2,
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frame_buffer] {}
            );
            break;
        default:
            RTC_LOG(LS_ERROR) << "frame_buffer type: " << static_cast<int32_t>(video_frame_buffer_type) << " is not supported!";
            // ReSharper disable once CppDFAUnreachableCode
            report_error();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        const webrtc::ColorSpace& color_space = input_image.ColorSpace() ? *input_image.ColorSpace() : extract_h264_color_space(av_context_.get());

        auto decoded_frame = webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(cropped_buffer)
            .set_rtp_timestamp(input_image.RtpTimestamp())
            .set_color_space(color_space)
            .build();

        decoded_image_callback_->Decoded(decoded_frame, std::nullopt, qp);
        av_frame_unref(av_frame_.get());
        input_frame = nullptr;
        return WEBRTC_VIDEO_CODEC_OK;
    }
} // openh264
#endif