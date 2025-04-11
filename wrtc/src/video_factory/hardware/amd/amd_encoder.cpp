//
// Created by Laky64 on 10/04/25.
//

#include <libyuv.h>
#include <modules/video_coding/include/video_codec_interface.h>
#include <modules/video_coding/include/video_error_codes.h>
#include <public/common/Thread.h>
#include <public/include/components/VideoEncoderAV1.h>
#include <public/include/components/VideoEncoderHEVC.h>
#include <public/include/components/VideoEncoderVCE.h>
#include <rtc_base/logging.h>
#include <wrtc/video_factory/hardware/amd/amd_encoder.hpp>


#define RETURN_IF_FAILED(res, message)                  \
if (res != AMF_OK) {                                  \
RTC_LOG(LS_ERROR) << amf::amf_from_unicode_to_utf8( \
amf::AMFFormatResult(res)) \
<< message;                       \
return res;                                         \
}
#define TRACE() RTC_LOG(LS_ERROR) << "TRACE: " << __LINE__

#define FRAME_RTP_TIMESTAMP_PROPERTY L"FRAME_RTP_TIMESTAMP_PROPERTY"
#define FRAME_NTP_TIME_MS_PROPERTY L"FRAME_NTP_TIME_MS_PROPERTY"
#define FRAME_RENDER_TIME_MS_PROPERTY L"FRAME_RENDER_TIME_MS_PROPERTY"
#define FRAME_ROTATION_PROPERTY L"FRAME_ROTATION_PROPERTY"
#define FRAME_COLOR_SPACE_PROPERTY L"FRAME_COLOR_SPACE_PROPERTY"

namespace amd {
    constexpr int kLowH264QpThreshold = 34;
    constexpr int kHighH264QpThreshold = 40;
#if defined(_WIN32)
    constexpr amf::AMF_MEMORY_TYPE kPlatformMemoryType = amf::AMF_MEMORY_DX11;
#else
    constexpr amf::AMF_MEMORY_TYPE kPlatformMemoryType = amf::AMF_MEMORY_VULKAN;
#endif

    AMDEncoder::AMDEncoder(const webrtc::VideoCodecType codec): codec(codec), bitrateAdjuster(0.5, 0.95) {}

    AMDEncoder::~AMDEncoder() {
        Release();
    }

    bool AMDEncoder::IsSupported(const webrtc::VideoCodecType codec) {
        const auto amfContext = AMDContext::GetOrCreateDefault();
        if (amfContext == nullptr) {
            return false;
        }

        amf::AMFContextPtr context;
        amf::AMFComponentPtr encoder;
        const auto res = CreateEncoder(
            amfContext,
            codec,
            640,
            480,
            30,
            100 * 1000,
            500 * 1000,
            kPlatformMemoryType,
            &context,
            &encoder
        );
        if (res != AMF_OK || context == nullptr || encoder == nullptr) {
            return false;
        }
        return true;
    }

    AMF_RESULT AMDEncoder::CreateEncoder(
        rtc::scoped_refptr<AMDContext> amdContext,
        const webrtc::VideoCodecType codec,
        const int width,
        const int height,
        const int framerate,
        const int targetBitrateBps,
        int,
        amf::AMF_MEMORY_TYPE memoryType,
        amf::AMFContext** outContext,
        amf::AMFComponent** outEncoder
    ){
        if (!(codec == webrtc::kVideoCodecAV1 || codec == webrtc::kVideoCodecH265 || codec == webrtc::kVideoCodecH264)) {
            return AMF_NOT_SUPPORTED;
        }

        amf::AMFContextPtr context;
        amf::AMFComponentPtr encoder;
        amf::AMFSurfacePtr surface;
        auto res = GetAMFFactoryHelper(amdContext)->GetFactory()->CreateContext(&context);
        RETURN_IF_FAILED(res, "Failed to CreateContext()");
        if (memoryType == amf::AMF_MEMORY_OPENGL) {
            res = context->InitOpenGL(nullptr, nullptr, nullptr);
            RETURN_IF_FAILED(res, "Failed to InitOpenGL()");
        } else if (memoryType == amf::AMF_MEMORY_VULKAN) {
            res = amf::AMFContext1Ptr(context)->InitVulkan(nullptr);
            RETURN_IF_FAILED(res, "Failed to InitVulkan()");
        }
#if defined(_WIN32)
        if (memoryType == amf::AMF_MEMORY_DX9) {
          res = context->InitDX9(nullptr);  // can be DX9 or DX9Ex device
          RETURN_IF_FAILED(res, "Failed to InitDX9(NULL)");
        } else if (memoryType == amf::AMF_MEMORY_DX11) {
          res = context->InitDX11(nullptr);  // can be DX11 device
          RETURN_IF_FAILED(res, "Failed to InitDX11(NULL)");
        } else if (memoryType == amf::AMF_MEMORY_DX12) {
          res = amf::AMFContext2Ptr(context)->InitDX12(nullptr);  // can be DX12 device
          RETURN_IF_FAILED(res, "Failed to InitDX12(NULL)");
        }
#endif

        const auto amfCodec = codec == webrtc::kVideoCodecH264   ? AMFVideoEncoderVCE_AVC : codec == webrtc::kVideoCodecH265 ? AMFVideoEncoder_HEVC : AMFVideoEncoder_AV1;
        res = GetAMFFactoryHelper(amdContext)->GetFactory()->CreateComponent(context, amfCodec, &encoder);
        RETURN_IF_FAILED(res, L"CreateComponent() failed");

        if (codec == webrtc::kVideoCodecAV1) {
            res = encoder->SetProperty(AMF_VIDEO_ENCODER_AV1_USAGE, AMF_VIDEO_ENCODER_AV1_USAGE_TRANSCODING);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_AV1_USAGE, " "AMF_VIDEO_ENCODER_AV1_USAGE_TRANSCODING)");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE, AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE_LOWEST_LATENCY);
            RETURN_IF_FAILED(res, "SetProperty(AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE, " "AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE_LOWEST_LATENCY) failed");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE, AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE_NO_RESTRICTIONS);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE, " "AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE_NO_RESTRICTIONS)");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_AV1_TARGET_BITRATE, targetBitrateBps);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_AV1_TARGET_BITRATE)");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_AV1_FRAMERATE, AMFConstructRate(framerate, 1));
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_AV1_FRAMERATE)");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_AV1_FRAMESIZE, AMFConstructSize(width, height));
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_AV1_FRAMESIZE");
        } else if (codec == webrtc::kVideoCodecH265) {
            res = encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_USAGE, AMF_VIDEO_ENCODER_HEVC_USAGE_TRANSCODING);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_HEVC_USAGE, " "AMF_VIDEO_ENCODER_HEVC_USAGE_TRANSCODING)");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_TARGET_BITRATE, targetBitrateBps);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_HEVC_TARGET_BITRATE)");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_FRAMERATE, AMFConstructRate(framerate, 1));
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_HEVC_FRAMERATE)");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_LOWLATENCY_MODE, true);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_HEVC_LOWLATENCY_MODE, true)");
        } else {
            res = encoder->SetProperty(AMF_VIDEO_ENCODER_USAGE, AMF_VIDEO_ENCODER_USAGE_TRANSCODING);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_USAGE, " "AMF_VIDEO_ENCODER_USAGE_TRANSCODING)");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_TARGET_BITRATE, targetBitrateBps);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_TARGET_BITRATE)");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_FRAMERATE, AMFConstructRate(framerate, 1));
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_FRAMERATE");

            res = encoder->SetProperty(AMF_VIDEO_ENCODER_LOWLATENCY_MODE, true);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_LOWLATENCY_MODE, true)");
        }

        res = encoder->Init(amf::AMF_SURFACE_YUV420P, width, height);
        RETURN_IF_FAILED(res, "Failed to encoder->Init()");

        *outContext = context.Detach();
        *outEncoder = encoder.Detach();

        return res;
    }

    int32_t AMDEncoder::RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) {
        std::lock_guard lock(mutex);
        callbackEncodedImage = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t AMDEncoder::InitEncode(const webrtc::VideoCodec* codecSettings, int32_t numberOfCores, size_t maxPayloadSize) {
        RTC_DCHECK(codecSettings);

        if (const int32_t releaseRet = Release(); releaseRet != WEBRTC_VIDEO_CODEC_OK) {
            return releaseRet;
        }

        width = codecSettings->width;
        height = codecSettings->height;
        targetBitrateBps = codecSettings->startBitrate * 1000;
        maxBitrateBps = codecSettings->maxBitrate * 1000;
        bitrateAdjuster.SetTargetBitrateBps(targetBitrateBps);
        framerate = codecSettings->maxFramerate;
        mode = codecSettings->mode;

        RTC_LOG(LS_INFO) << "InitEncode " << targetBitrateBps << "bit/sec";

        if (codecSettings->codecType == webrtc::kVideoCodecAV1) {
            auto scalability_mode = codecSettings->GetScalabilityMode();
            if (!scalability_mode) {
                RTC_LOG(LS_WARNING) << "Scalability mode is not set, using 'L1T1'.";
                scalability_mode = webrtc::ScalabilityMode::kL1T1;
            }
            RTC_LOG(LS_INFO) << "InitEncode scalability_mode:" << static_cast<int>(*scalability_mode);
            svcController = CreateScalabilityStructure(*scalability_mode);
            scalabilityMode = *scalability_mode;
        }

        if (InitAMF() != AMF_OK) {
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t AMDEncoder::Release() {
        ReleaseAMF();
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t AMDEncoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frameTypes) {
        if (encoder == nullptr) {
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        if (!callbackEncodedImage) {
            RTC_LOG(LS_WARNING) << "InitEncode() has been called, but a callback function has not been set with RegisterEncodeCompleteCallback()";
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }

        AMF_RESULT res;
        if (reconfigureNeeded) {
            if (codec == webrtc::kVideoCodecAV1) {
                res = encoder->SetProperty(AMF_VIDEO_ENCODER_AV1_TARGET_BITRATE, targetBitrateBps);
                RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_AV1_TARGET_BITRATE)");
            } else if (codec == webrtc::kVideoCodecH265) {
                res = encoder->SetProperty(AMF_VIDEO_ENCODER_HEVC_TARGET_BITRATE, targetBitrateBps);
                RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_HEVC_TARGET_BITRATE)");
            } else if (codec == webrtc::kVideoCodecH264) {
                res = encoder->SetProperty(AMF_VIDEO_ENCODER_TARGET_BITRATE, targetBitrateBps);
                RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_TARGET_BITRATE)");
            }
            reconfigureNeeded = false;
        }

        if (surface == nullptr) {
            res = context->AllocSurface(amf::AMF_MEMORY_HOST, amf::AMF_SURFACE_YUV420P, static_cast<int>(width), static_cast<int>(height), &surface);
            RETURN_IF_FAILED(res, "Failed to AllocSurface");
        }
        {
            amf::AMFPlane* py = surface->GetPlane(amf::AMF_PLANE_Y);
            amf::AMFPlane* pu = surface->GetPlane(amf::AMF_PLANE_U);
            amf::AMFPlane* pv = surface->GetPlane(amf::AMF_PLANE_V);
            const auto src = frame.video_frame_buffer()->ToI420();
            libyuv::I420Copy(
                src->DataY(),
                src->StrideY(),
                src->DataU(),
                src->StrideU(),
                src->DataV(),
                src->StrideV(),
                static_cast<uint8_t*>(py->GetNative()),
                py->GetHPitch(),
                static_cast<uint8_t*>(pu->GetNative()),
                pu->GetHPitch(),
                static_cast<uint8_t*>(pv->GetNative()),
                pv->GetHPitch(),
                src->width(),
                src->height()
            );

            res = surface->SetProperty(FRAME_RTP_TIMESTAMP_PROPERTY, frame.rtp_timestamp());
            RETURN_IF_FAILED(res, "Failed to SetProperty(FRAME_RTP_TIMESTAMP_PROPERTY)");

            res = surface->SetProperty(FRAME_NTP_TIME_MS_PROPERTY, frame.ntp_time_ms());
            RETURN_IF_FAILED(res, "Failed to SetProperty(FRAME_NTP_TIME_MS_PROPERTY)");

            res = surface->SetProperty(FRAME_RENDER_TIME_MS_PROPERTY, frame.render_time_ms());
            RETURN_IF_FAILED(res, "Failed to SetProperty(FRAME_RENDER_TIME_MS_PROPERTY)");

            res = surface->SetProperty(FRAME_ROTATION_PROPERTY, static_cast<int64_t>(frame.rotation()));
            RETURN_IF_FAILED(res, "Failed to SetProperty(FRAME_ROTATION_PROPERTY)");
        }

        bool sendKeyFrame = false;
        if (frameTypes != nullptr) {
            RTC_DCHECK_EQ(frameTypes->size(), static_cast<size_t>(1));
            if ((*frameTypes)[0] == webrtc::VideoFrameType::kEmptyFrame) {
              return WEBRTC_VIDEO_CODEC_OK;
            }
            sendKeyFrame = (*frameTypes)[0] == webrtc::VideoFrameType::kVideoFrameKey;
        }

        if (sendKeyFrame) {
            res = surface->SetProperty(AMF_VIDEO_ENCODER_FORCE_PICTURE_TYPE, AMF_VIDEO_ENCODER_PICTURE_TYPE_IDR);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_FORCE_PICTURE_TYPE)");

            res = surface->SetProperty(AMF_VIDEO_ENCODER_INSERT_SPS, true);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_INSERT_SPS)");

            res = surface->SetProperty(AMF_VIDEO_ENCODER_INSERT_PPS, true);
            RETURN_IF_FAILED(res, "Failed to SetProperty(AMF_VIDEO_ENCODER_INSERT_PPS)");
        }

        res = encoder->SubmitInput(surface);
        if (res == AMF_NEED_MORE_INPUT) {
        } else if (res == AMF_INPUT_FULL || res == AMF_DECODER_NO_FREE_SURFACES) {
            amf_sleep(1);
        } else {
            RETURN_IF_FAILED(res, L"Failed to SubmitInput()");
            surface = nullptr;
        }

        return WEBRTC_VIDEO_CODEC_OK;
    }

    void AMDEncoder::SetRates(const RateControlParameters& parameters) {
        if (encoder == nullptr) {
            RTC_LOG(LS_WARNING) << "SetRates() while uninitialized.";
            return;
        }

        if (parameters.framerate_fps < 1.0) {
            RTC_LOG(LS_WARNING) << "Invalid frame rate: " << parameters.framerate_fps;
            return;
        }

        if (svcController) {
            svcController->OnRatesUpdated(parameters.bitrate);
        }

        const auto newFramerate = static_cast<uint32_t>(parameters.framerate_fps);
        const auto newBitrate = parameters.bitrate.get_sum_bps();
        RTC_LOG(LS_INFO) << __FUNCTION__ << " framerate:" << framerate
                         << " newFramerate: " << newFramerate
                         << " targetBitrateBps:" << targetBitrateBps
                         << " newBitrate:" << newBitrate
                         << " maxBitrateBps:" << maxBitrateBps;
        framerate = newFramerate;
        targetBitrateBps = newBitrate;
        bitrateAdjuster.SetTargetBitrateBps(targetBitrateBps);
        reconfigureNeeded = true;
    }

    webrtc::VideoEncoder::EncoderInfo AMDEncoder::GetEncoderInfo() const {
        EncoderInfo info;
        info.supports_native_handle = true;
        info.implementation_name = "AMF";
        info.scaling_settings = ScalingSettings(kLowH264QpThreshold, kHighH264QpThreshold);
        return info;
    }

    AMF_RESULT AMDEncoder::InitAMF() {
        reconfigureNeeded = false;

        memoryType = kPlatformMemoryType;

        const auto encoderRes = CreateEncoder(
            AMDContext::GetOrCreateDefault(),
            codec,
            static_cast<int>(width),
            static_cast<int>(height),
            static_cast<int>(framerate),
            static_cast<int>(targetBitrateBps),
            static_cast<int>(maxBitrateBps),
            memoryType,
            &context,
            &encoder
        );
        RETURN_IF_FAILED(encoderRes, "Failed to CreateEncoder()");

        pollingThread = std::make_unique<std::thread>([this, codec = codec] {
            AMF_RESULT res = AMF_OK;
            while (true) {
                amf::AMFDataPtr data;
                res = encoder->QueryOutput(&data);
                if (res == AMF_EOF) {
                    break;
                }
                if (res != AMF_OK && res != AMF_REPEAT) {
                    break;
                }
                if (data == nullptr) {
                    amf_sleep(1);
                    continue;
                }
                amf::AMFBufferPtr buffer(data);
                ProcessBuffer(buffer, codec);
            }
        });
        return AMF_OK;
    }

    AMF_RESULT AMDEncoder::ProcessBuffer(const amf::AMFBufferPtr& buffer, const webrtc::VideoCodecType codec) {
        AMF_RESULT res = AMF_OK;
        uint32_t rtpTimestamp;
        int64_t ntpTimeMs;
        int64_t renderTimeMs;
        int64_t rotation;

        res = buffer->GetProperty(FRAME_RTP_TIMESTAMP_PROPERTY, &rtpTimestamp);
        RETURN_IF_FAILED(res, "Failed to GetProperty(FRAME_RTP_TIMESTAMP_PROPERTY)");

        res = buffer->GetProperty(FRAME_NTP_TIME_MS_PROPERTY, &ntpTimeMs);
        RETURN_IF_FAILED(res, "Failed to GetProperty(FRAME_NTP_TIME_MS_PROPERTY)");

        res = buffer->GetProperty(FRAME_RENDER_TIME_MS_PROPERTY, &renderTimeMs);
        RETURN_IF_FAILED(res, "Failed to GetProperty(FRAME_RENDER_TIME_MS_PROPERTY)");

        res = buffer->GetProperty(FRAME_ROTATION_PROPERTY, &rotation);
        RETURN_IF_FAILED(res, "Failed to GetProperty(FRAME_ROTATION_PROPERTY)");

        const auto p = static_cast<uint8_t*>(buffer->GetNative());
        const auto size = buffer->GetSize();
        const auto encodedImageBuffer = webrtc::EncodedImageBuffer::Create(p, size);
        encodedImage.SetEncodedData(encodedImageBuffer);
        encodedImage._encodedWidth = width;
        encodedImage._encodedHeight = height;
        encodedImage.content_type_ =
            mode == webrtc::VideoCodecMode::kScreensharing
                ? webrtc::VideoContentType::SCREENSHARE
                : webrtc::VideoContentType::UNSPECIFIED;
        encodedImage.timing_.flags = webrtc::VideoSendTiming::kInvalid;
        encodedImage.SetRtpTimestamp(rtpTimestamp);
        encodedImage.ntp_time_ms_ = ntpTimeMs;
        encodedImage.capture_time_ms_ = renderTimeMs;
        encodedImage.rotation_ = static_cast<webrtc::VideoRotation>(rotation);
        encodedImage._frameType = webrtc::VideoFrameType::kVideoFrameDelta;

        if (codec == webrtc::kVideoCodecH264) {
            int64_t dataType;
            res = buffer->GetProperty(AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE, &dataType);
            RETURN_IF_FAILED(res, "Failed to GetProperty(AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE)");
            if (dataType == AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_IDR || dataType == AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_I) {
                encodedImage._frameType = webrtc::VideoFrameType::kVideoFrameKey;
            }
        } else if (codec == webrtc::kVideoCodecH265) {
            int64_t dataType;
            res = buffer->GetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE, &dataType);
            RETURN_IF_FAILED(
                res, "Failed to GetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE)");
            if (dataType == AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_IDR || dataType == AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_I) {
              encodedImage._frameType = webrtc::VideoFrameType::kVideoFrameKey;
            }
        } else if (codec == webrtc::kVideoCodecAV1) {
            int64_t data_type;
            res = buffer->GetProperty(AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE, &data_type);
            RETURN_IF_FAILED(res, "Failed to GetProperty(AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE)");
            if (data_type == AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_KEY) {
                encodedImage._frameType = webrtc::VideoFrameType::kVideoFrameKey;
            }
        }

        webrtc::CodecSpecificInfo codecSpecific;
        if (codec == webrtc::kVideoCodecH264) {
            codecSpecific.codecType = webrtc::kVideoCodecH264;
            codecSpecific.codecSpecific.H264.packetization_mode = webrtc::H264PacketizationMode::NonInterleaved;
            h264BitstreamParser.ParseBitstream(encodedImage);
            encodedImage.qp_ = h264BitstreamParser.GetLastSliceQp().value_or(-1);
        } else if (codec == webrtc::kVideoCodecH265) {
            codecSpecific.codecType = webrtc::kVideoCodecH265;
            h265BitstreamParser.ParseBitstream(encodedImage);
            encodedImage.qp_ = h265BitstreamParser.GetLastSliceQp().value_or(-1);
        } else if (codec == webrtc::kVideoCodecAV1) {
            codecSpecific.codecType = webrtc::kVideoCodecAV1;

            const bool isKey = encodedImage._frameType == webrtc::VideoFrameType::kVideoFrameKey;
            const auto layerFrames = svcController->NextFrameConfig(isKey);
            if (layerFrames.empty()) {
              return AMF_OK;
            }
            codecSpecific.end_of_picture = true;
            codecSpecific.scalability_mode = scalabilityMode;
            codecSpecific.generic_frame_info = svcController->OnEncodeDone(layerFrames[0]);
            if (isKey && codecSpecific.generic_frame_info) {
                codecSpecific.template_structure = svcController->DependencyStructure();
                auto& resolutions = codecSpecific.template_structure->resolutions;
                resolutions = {
                    webrtc::RenderResolution(static_cast<int>(encodedImage._encodedWidth), static_cast<int>(encodedImage._encodedHeight))
                };
            }
        }

        if (const auto result = callbackEncodedImage->OnEncodedImage(encodedImage, &codecSpecific); result.error != webrtc::EncodedImageCallback::Result::OK) {
            RTC_LOG(LS_ERROR) << __FUNCTION__ << " OnEncodedImage failed error:" << result.error;
            return AMF_FAIL;
        }
        bitrateAdjuster.Update(size);
        return AMF_OK;
    }

    AMF_RESULT AMDEncoder::ReleaseAMF() {
        if (encoder != nullptr) {
            encoder->Drain();
        }
        if (pollingThread != nullptr) {
            pollingThread->join();
        }

        encoder = nullptr;
        pollingThread.reset();

        return AMF_OK;
    }
} // amd