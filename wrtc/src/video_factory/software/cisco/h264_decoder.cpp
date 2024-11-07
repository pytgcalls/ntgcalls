//
// Created by Laky64 on 05/11/24.
//

#ifndef IS_ANDROID
// ReSharper disable CppDFAUnusedValue
#include <modules/video_coding/include/video_error_codes.h>
#include <rtc_base/logging.h>
#include <system_wrappers/include/metrics.h>
#include <wrtc/video_factory/software/openh264/h264_decoder.hpp>
#include <common_video/include/video_frame_buffer.h>
#include <wrtc/video_factory/software/openh264/h264_color_space.hpp>

extern "C" {
    #include <libavutil/imgutils.h>
}

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
        : ffmpegBufferPool(true),
          decodedImageCallback(nullptr),
          hasReportedInit(false),
          hasReportedError(false) {}

    H264Decoder::~H264Decoder() {
        Release();
    }

    bool H264Decoder::Configure(const Settings& settings) {
        ReportInit();
        if (settings.codec_type() != webrtc::kVideoCodecH264) {
            ReportError();
            return false;
        }
        if (const int32_t ret = Release(); ret != WEBRTC_VIDEO_CODEC_OK) {
            ReportError();
            return false;
        }
        RTC_DCHECK(!avContext);
        avContext.reset(avcodec_alloc_context3(nullptr));
        avContext->codec_type = AVMEDIA_TYPE_VIDEO;
        avContext->codec_id = AV_CODEC_ID_H264;
        const webrtc::RenderResolution& resolution = settings.max_render_resolution();
        if (resolution.Valid()) {
            avContext->coded_width = resolution.Width();
            avContext->coded_height = resolution.Height();
        }
        avContext->extradata = nullptr;
        avContext->extradata_size = 0;

        avContext->thread_count = 1;
        avContext->thread_type = FF_THREAD_SLICE;

        avContext->get_buffer2 = AVGetBuffer2;

        avContext->opaque = this;

        const AVCodec* codec = avcodec_find_decoder(avContext->codec_id);
        if (!codec) {
            RTC_LOG(LS_ERROR) << "FFmpeg H.264 decoder not found.";
            Release();
            ReportError();
            return false;
        }
        if (const int res = avcodec_open2(avContext.get(), codec, nullptr); res < 0) {
            RTC_LOG(LS_ERROR) << "avcodec_open2 error: " << res;
            Release();
            ReportError();
            return false;
        }

        avFrame.reset(av_frame_alloc());

        if (const std::optional<int> bufferPoolSize = settings.buffer_pool_size()) {
            if (!ffmpegBufferPool.Resize(*bufferPoolSize)) {
                return false;
            }
        }
        return true;
    }

    int32_t H264Decoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) {
        decodedImageCallback = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t H264Decoder::Release() {
        avContext.reset();
        avFrame.reset();
        return WEBRTC_VIDEO_CODEC_OK;
    }

    void H264Decoder::ReportInit() {
        if (hasReportedInit)
            return;
        RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.H264DecoderImpl.Event", kH264DecoderEventInit, kH264DecoderEventMax);
        hasReportedInit = true;
    }

    void H264Decoder::ReportError() {
        if (hasReportedError)
            return;
        RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.H264DecoderImpl.Event", kH264DecoderEventError, kH264DecoderEventMax);
        hasReportedError = true;
    }

    int H264Decoder::AVGetBuffer2(AVCodecContext* context, AVFrame* avFrame, int flags) {
        auto* decoder = static_cast<H264Decoder*>(context->opaque);
        RTC_DCHECK(decoder);
        RTC_DCHECK(context->codec->capabilities | AV_CODEC_CAP_DR1);

        const auto pixelFormatSupported = std::ranges::find_if(kPixelFormatsSupported, [context](const AVPixelFormat format) {
            return context->pix_fmt == format;
        });

        if (pixelFormatSupported == kPixelFormatsSupported.end()) {
            RTC_LOG(LS_ERROR) << "Unsupported pixel format: " << context->pix_fmt;
            decoder->ReportError();
            return -1;
        }

        int width = avFrame->width;
        int height = avFrame->height;

        RTC_CHECK_EQ(context->lowres, 0);

        avcodec_align_dimensions(context, &width, &height);

        RTC_CHECK_GE(width, 0);
        RTC_CHECK_GE(height, 0);
        if (const int ret = av_image_check_size(static_cast<unsigned int>(width), static_cast<unsigned int>(height), 0, nullptr); ret < 0) {
            RTC_LOG(LS_ERROR) << "Invalid picture size " << width << "x" << height;
            decoder->ReportError();
            return ret;
        }

        rtc::scoped_refptr<webrtc::PlanarYuvBuffer> frameBuffer;
        rtc::scoped_refptr<webrtc::I444Buffer> i444Buffer;
        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer;
        rtc::scoped_refptr<webrtc::I422Buffer> i422Buffer;
        rtc::scoped_refptr<webrtc::I010Buffer> i010Buffer;
        rtc::scoped_refptr<webrtc::I210Buffer> i210Buffer;
        rtc::scoped_refptr<webrtc::I410Buffer> i410Buffer;
        int bytes_per_pixel = 1;
        switch (context->pix_fmt) {
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P:
            i420Buffer = decoder->ffmpegBufferPool.CreateI420Buffer(width, height);
            avFrame->data[kYPlaneIndex] = i420Buffer->MutableDataY();
            avFrame->linesize[kYPlaneIndex] = i420Buffer->StrideY();
            avFrame->data[kUPlaneIndex] = i420Buffer->MutableDataU();
            avFrame->linesize[kUPlaneIndex] = i420Buffer->StrideU();
            avFrame->data[kVPlaneIndex] = i420Buffer->MutableDataV();
            avFrame->linesize[kVPlaneIndex] = i420Buffer->StrideV();
            RTC_DCHECK_EQ(avFrame->extended_data, avFrame->data);
            frameBuffer = i420Buffer;
            break;
        case AV_PIX_FMT_YUV444P:
        case AV_PIX_FMT_YUVJ444P:
            i444Buffer = decoder->ffmpegBufferPool.CreateI444Buffer(width, height);
            avFrame->data[kYPlaneIndex] = i444Buffer->MutableDataY();
            avFrame->linesize[kYPlaneIndex] = i444Buffer->StrideY();
            avFrame->data[kUPlaneIndex] = i444Buffer->MutableDataU();
            avFrame->linesize[kUPlaneIndex] = i444Buffer->StrideU();
            avFrame->data[kVPlaneIndex] = i444Buffer->MutableDataV();
            avFrame->linesize[kVPlaneIndex] = i444Buffer->StrideV();
            frameBuffer = i444Buffer;
            break;
        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUVJ422P:
            i422Buffer = decoder->ffmpegBufferPool.CreateI422Buffer(width, height);
            avFrame->data[kYPlaneIndex] = i422Buffer->MutableDataY();
            avFrame->linesize[kYPlaneIndex] = i422Buffer->StrideY();
            avFrame->data[kUPlaneIndex] = i422Buffer->MutableDataU();
            avFrame->linesize[kUPlaneIndex] = i422Buffer->StrideU();
            avFrame->data[kVPlaneIndex] = i422Buffer->MutableDataV();
            avFrame->linesize[kVPlaneIndex] = i422Buffer->StrideV();
            frameBuffer = i422Buffer;
            break;
        case AV_PIX_FMT_YUV420P10LE:
            i010Buffer = decoder->ffmpegBufferPool.CreateI010Buffer(width, height);
            avFrame->data[kYPlaneIndex] = reinterpret_cast<uint8_t*>(i010Buffer->MutableDataY());
            avFrame->linesize[kYPlaneIndex] = i010Buffer->StrideY() * 2;
            avFrame->data[kUPlaneIndex] = reinterpret_cast<uint8_t*>(i010Buffer->MutableDataU());
            avFrame->linesize[kUPlaneIndex] = i010Buffer->StrideU() * 2;
            avFrame->data[kVPlaneIndex] = reinterpret_cast<uint8_t*>(i010Buffer->MutableDataV());
            avFrame->linesize[kVPlaneIndex] = i010Buffer->StrideV() * 2;
            frameBuffer = i010Buffer;
            bytes_per_pixel = 2;
            break;
        case AV_PIX_FMT_YUV422P10LE:
            i210Buffer = decoder->ffmpegBufferPool.CreateI210Buffer(width, height);
            avFrame->data[kYPlaneIndex] = reinterpret_cast<uint8_t*>(i210Buffer->MutableDataY());
            avFrame->linesize[kYPlaneIndex] = i210Buffer->StrideY() * 2;
            avFrame->data[kUPlaneIndex] = reinterpret_cast<uint8_t*>(i210Buffer->MutableDataU());
            avFrame->linesize[kUPlaneIndex] = i210Buffer->StrideU() * 2;
            avFrame->data[kVPlaneIndex] = reinterpret_cast<uint8_t*>(i210Buffer->MutableDataV());
            avFrame->linesize[kVPlaneIndex] = i210Buffer->StrideV() * 2;
            frameBuffer = i210Buffer;
            bytes_per_pixel = 2;
            break;
        case AV_PIX_FMT_YUV444P10LE:
            i410Buffer = decoder->ffmpegBufferPool.CreateI410Buffer(width, height);
            avFrame->data[kYPlaneIndex] = reinterpret_cast<uint8_t*>(i410Buffer->MutableDataY());
            avFrame->linesize[kYPlaneIndex] = i410Buffer->StrideY() * 2;
            avFrame->data[kUPlaneIndex] = reinterpret_cast<uint8_t*>(i410Buffer->MutableDataU());
            avFrame->linesize[kUPlaneIndex] = i410Buffer->StrideU() * 2;
            avFrame->data[kVPlaneIndex] = reinterpret_cast<uint8_t*>(i410Buffer->MutableDataV());
            avFrame->linesize[kVPlaneIndex] = i410Buffer->StrideV() * 2;
            frameBuffer = i410Buffer;
            bytes_per_pixel = 2;
            break;
        default:
            RTC_LOG(LS_ERROR) << "Unsupported buffer type " << context->pix_fmt
                              << ". Check supported supported pixel formats!";
            decoder->ReportError();
            return -1;
        }

        const int ySize = width * height * bytes_per_pixel;
        const int uvSize = frameBuffer->ChromaWidth() * frameBuffer->ChromaHeight() * bytes_per_pixel;
        RTC_DCHECK_EQ(avFrame->data[kUPlaneIndex], avFrame->data[kYPlaneIndex] + ySize);
        RTC_DCHECK_EQ(avFrame->data[kVPlaneIndex], avFrame->data[kUPlaneIndex] + uvSize);
        const int totalSize = ySize + 2 * uvSize;

        avFrame->format = context->pix_fmt;
        avFrame->buf[0] = av_buffer_create(
            avFrame->data[kYPlaneIndex], totalSize, AVFreeBuffer2,
            std::make_unique<webrtc::VideoFrame>(webrtc::VideoFrame::Builder()
                .set_video_frame_buffer(frameBuffer)
                .set_rotation(webrtc::kVideoRotation_0)
                .set_timestamp_us(0)
                .build())
            .release(),
            0);
        RTC_CHECK(avFrame->buf[0]);
        return 0;
    }

    void H264Decoder::AVFreeBuffer2(void* opaque, uint8_t*) {
        const auto* videoFrame = static_cast<webrtc::VideoFrame*>(opaque);
        delete videoFrame;
    }

    bool H264Decoder::IsInitialized() const {
        return avContext != nullptr;
    }

    int32_t H264Decoder::Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs) {
        if (!IsInitialized()) {
            ReportError();
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        if (!decodedImageCallback) {
            RTC_LOG(LS_WARNING)
                << "Configure() has been called, but a callback function "
                   "has not been set with RegisterDecodeCompleteCallback()";
            ReportError();
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }

        if (!inputImage.data() || !inputImage.size()) {
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }

        ScopedAVPacket packet = MakeScopedAVPacket();
        if (!packet) {
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        packet->data = const_cast<uint8_t*>(inputImage.data());
        if (inputImage.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        packet->size = static_cast<int>(inputImage.size());
        int result = avcodec_send_packet(avContext.get(), packet.get());
        if (result < 0) {
            RTC_LOG(LS_ERROR) << "avcodec_send_packet error: " << result;
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        result = avcodec_receive_frame(avContext.get(), avFrame.get());
        if (result < 0) {
            RTC_LOG(LS_ERROR) << "avcodec_receive_frame error: " << result;
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        h264BitstreamParser.ParseBitstream(inputImage);
        std::optional<int> qp = h264BitstreamParser.GetLastSliceQp();

        const auto* inputFrame = static_cast<webrtc::VideoFrame*>(av_buffer_get_opaque(avFrame->buf[0]));
        RTC_DCHECK(inputFrame);
        rtc::scoped_refptr<webrtc::VideoFrameBuffer> frameBuffer = inputFrame->video_frame_buffer();

        const webrtc::PlanarYuvBuffer* planarYuvBuffer = nullptr;
        const webrtc::PlanarYuv8Buffer* planarYuv8Buffer = nullptr;
        const webrtc::PlanarYuv16BBuffer* planarYuv16Buffer = nullptr;
        webrtc::VideoFrameBuffer::Type videoFrameBufferType = frameBuffer->type();
        
        switch (videoFrameBufferType) {
        case webrtc::VideoFrameBuffer::Type::kI420:
            planarYuvBuffer = frameBuffer->GetI420();
            planarYuv8Buffer = reinterpret_cast<const webrtc::PlanarYuv8Buffer*>(planarYuvBuffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI444:
            planarYuvBuffer = frameBuffer->GetI444();
            planarYuv8Buffer = reinterpret_cast<const webrtc::PlanarYuv8Buffer*>(planarYuvBuffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI422:
            planarYuvBuffer = frameBuffer->GetI422();
            planarYuv8Buffer = reinterpret_cast<const webrtc::PlanarYuv8Buffer*>(planarYuvBuffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI010:
            planarYuvBuffer = frameBuffer->GetI010();
            planarYuv16Buffer = reinterpret_cast<const webrtc::PlanarYuv16BBuffer*>(planarYuvBuffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI210:
            planarYuvBuffer = frameBuffer->GetI210();
            planarYuv16Buffer = reinterpret_cast<const webrtc::PlanarYuv16BBuffer*>(planarYuvBuffer);
            break;
        case webrtc::VideoFrameBuffer::Type::kI410:
            planarYuvBuffer = frameBuffer->GetI410();
            planarYuv16Buffer = reinterpret_cast<const webrtc::PlanarYuv16BBuffer*>(planarYuvBuffer);
            break;
        default:
            RTC_LOG(LS_ERROR) << "frame_buffer type: " << static_cast<int32_t>(videoFrameBufferType) << " is not supported!";
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        RTC_DCHECK_LE(avFrame->width, planarYuvBuffer->width());
        RTC_DCHECK_LE(avFrame->height, planarYuvBuffer->height());

        switch (videoFrameBufferType) {
        case webrtc::VideoFrameBuffer::Type::kI420:
        case webrtc::VideoFrameBuffer::Type::kI444:
        case webrtc::VideoFrameBuffer::Type::kI422: {
            RTC_DCHECK_GE(avFrame->data[kYPlaneIndex], planarYuv8Buffer->DataY());
            RTC_DCHECK_LE(
                avFrame->data[kYPlaneIndex] + avFrame->linesize[kYPlaneIndex] * avFrame->height,
                planarYuv8Buffer->DataY() +
                planarYuv8Buffer->StrideY() * planarYuv8Buffer->height()
            );
            RTC_DCHECK_GE(avFrame->data[kUPlaneIndex], planarYuv8Buffer->DataU());
            RTC_DCHECK_LE(
                avFrame->data[kUPlaneIndex] + avFrame->linesize[kUPlaneIndex] * planarYuv8Buffer->ChromaHeight(),
                planarYuv8Buffer->DataU() + planarYuv8Buffer->StrideU() * planarYuv8Buffer->ChromaHeight()
            );
            RTC_DCHECK_GE(avFrame->data[kVPlaneIndex], planarYuv8Buffer->DataV());
            RTC_DCHECK_LE(
                avFrame->data[kVPlaneIndex] + avFrame->linesize[kVPlaneIndex] * planarYuv8Buffer->ChromaHeight(),
                planarYuv8Buffer->DataV() + planarYuv8Buffer->StrideV() * planarYuv8Buffer->ChromaHeight()
            );
            break;
        }
        case webrtc::VideoFrameBuffer::Type::kI010:
        case webrtc::VideoFrameBuffer::Type::kI210:
        case webrtc::VideoFrameBuffer::Type::kI410: {
            RTC_DCHECK_GE(avFrame->data[kYPlaneIndex], reinterpret_cast<const uint8_t*>(planarYuv16Buffer->DataY()));
            RTC_DCHECK_LE(
                avFrame->data[kYPlaneIndex] + avFrame->linesize[kYPlaneIndex] * avFrame->height,
                reinterpret_cast<const uint8_t*>(planarYuv16Buffer->DataY()) + planarYuv16Buffer->StrideY() * 2 * planarYuv16Buffer->height()
            );
            RTC_DCHECK_GE(avFrame->data[kUPlaneIndex], reinterpret_cast<const uint8_t*>(planarYuv16Buffer->DataU()));
            RTC_DCHECK_LE(
                avFrame->data[kUPlaneIndex] + avFrame->linesize[kUPlaneIndex] * planarYuv16Buffer->ChromaHeight(),
                reinterpret_cast<const uint8_t*>(planarYuv16Buffer->DataU()) + planarYuv16Buffer->StrideU() * 2 * planarYuv16Buffer->ChromaHeight()
            );
            RTC_DCHECK_GE(avFrame->data[kVPlaneIndex], reinterpret_cast<const uint8_t*>(planarYuv16Buffer->DataV()));
            RTC_DCHECK_LE(
                avFrame->data[kVPlaneIndex] + avFrame->linesize[kVPlaneIndex] * planarYuv16Buffer->ChromaHeight(),
                reinterpret_cast<const uint8_t*>(planarYuv16Buffer->DataV()) + planarYuv16Buffer->StrideV() * 2 * planarYuv16Buffer->ChromaHeight()
            );
            break;
        }
        default:
            RTC_LOG(LS_ERROR) << "frame_buffer type: " << static_cast<int32_t>(videoFrameBufferType) << " is not supported!";
            // ReSharper disable once CppDFAUnreachableCode
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        rtc::scoped_refptr<webrtc::VideoFrameBuffer> croppedBuffer;
        switch (videoFrameBufferType) {
        case webrtc::VideoFrameBuffer::Type::kI420:
            croppedBuffer = webrtc::WrapI420Buffer(
                avFrame->width, avFrame->height, avFrame->data[kYPlaneIndex],
                avFrame->linesize[kYPlaneIndex], avFrame->data[kUPlaneIndex],
                avFrame->linesize[kUPlaneIndex], avFrame->data[kVPlaneIndex],
                avFrame->linesize[kVPlaneIndex],
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frameBuffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI444:
            croppedBuffer = webrtc::WrapI444Buffer(
                avFrame->width, avFrame->height, avFrame->data[kYPlaneIndex],
                avFrame->linesize[kYPlaneIndex], avFrame->data[kUPlaneIndex],
                avFrame->linesize[kUPlaneIndex], avFrame->data[kVPlaneIndex],
                avFrame->linesize[kVPlaneIndex],
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frameBuffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI422:
            croppedBuffer = webrtc::WrapI422Buffer(
                avFrame->width, avFrame->height, avFrame->data[kYPlaneIndex],
                avFrame->linesize[kYPlaneIndex], avFrame->data[kUPlaneIndex],
                avFrame->linesize[kUPlaneIndex], avFrame->data[kVPlaneIndex],
                avFrame->linesize[kVPlaneIndex],
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frameBuffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI010:
            croppedBuffer = webrtc::WrapI010Buffer(
                avFrame->width, avFrame->height,
                reinterpret_cast<const uint16_t*>(avFrame->data[kYPlaneIndex]),
                avFrame->linesize[kYPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(avFrame->data[kUPlaneIndex]),
                avFrame->linesize[kUPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(avFrame->data[kVPlaneIndex]),
                avFrame->linesize[kVPlaneIndex] / 2,
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frameBuffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI210:
            croppedBuffer = webrtc::WrapI210Buffer(
                avFrame->width, avFrame->height,
                reinterpret_cast<const uint16_t*>(avFrame->data[kYPlaneIndex]),
                avFrame->linesize[kYPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(avFrame->data[kUPlaneIndex]),
                avFrame->linesize[kUPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(avFrame->data[kVPlaneIndex]),
                avFrame->linesize[kVPlaneIndex] / 2,
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frameBuffer] {}
            );
            break;
        case webrtc::VideoFrameBuffer::Type::kI410:
            croppedBuffer = webrtc::WrapI410Buffer(
                avFrame->width, avFrame->height,
                reinterpret_cast<const uint16_t*>(avFrame->data[kYPlaneIndex]),
                avFrame->linesize[kYPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(avFrame->data[kUPlaneIndex]),
                avFrame->linesize[kUPlaneIndex] / 2,
                reinterpret_cast<const uint16_t*>(avFrame->data[kVPlaneIndex]),
                avFrame->linesize[kVPlaneIndex] / 2,
                // ReSharper disable once CppLambdaCaptureNeverUsed
                [frameBuffer] {}
            );
            break;
        default:
            RTC_LOG(LS_ERROR) << "frame_buffer type: " << static_cast<int32_t>(videoFrameBufferType) << " is not supported!";
            // ReSharper disable once CppDFAUnreachableCode
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }

        const webrtc::ColorSpace& color_space = inputImage.ColorSpace() ? *inputImage.ColorSpace() : ExtractH264ColorSpace(avContext.get());

        auto decoded_frame = webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(croppedBuffer)
            .set_rtp_timestamp(inputImage.RtpTimestamp())
            .set_color_space(color_space)
            .build();

        decodedImageCallback->Decoded(decoded_frame, std::nullopt, qp);
        av_frame_unref(avFrame.get());
        inputFrame = nullptr;
        return WEBRTC_VIDEO_CODEC_OK;
    }
} // openh264
#endif