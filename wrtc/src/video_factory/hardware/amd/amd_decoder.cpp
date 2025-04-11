//
// Created by Laky64 on 11/04/25.
//


#include <libyuv.h>
#include <modules/video_coding/include/video_error_codes.h>

#include "public/common/AMFFactory.h"
#include "public/common/AMFSTL.h"
#include "public/common/Thread.h"
#include <public/include/components/VideoDecoderUVD.h>
#include <rtc_base/logging.h>
#include <wrtc/video_factory/hardware/amd/amd_decoder.hpp>

#define RETURN_IF_FAILED(res, message)                  \
if (res != AMF_OK) {                                  \
RTC_LOG(LS_ERROR) << amf::amf_from_unicode_to_utf8( \
amf::AMFFormatResult(res)) \
<< message;                       \
return res;                                         \
}
#define TRACE() RTC_LOG(LS_ERROR) << "TRACE: " << __LINE__

namespace amd {
#if defined(_WIN32)
    constexpr amf::AMF_MEMORY_TYPE kPlatformMemoryType = amf::AMF_MEMORY_DX11;
#else
    constexpr amf::AMF_MEMORY_TYPE kPlatformMemoryType = amf::AMF_MEMORY_VULKAN;
#endif

    AMDDecoder::AMDDecoder(const webrtc::VideoCodecType codec): codec(codec), bufferPool(false, 300) {}

    AMDDecoder::~AMDDecoder() {
        Release();
    }

    bool AMDDecoder::IsSupported(const webrtc::VideoCodecType codec) {
        const auto amfContext = AMDContext::GetOrCreateDefault();
        if (amfContext == nullptr) {
            return false;
        }

        amf::AMFContextPtr context;
        amf::AMFComponentPtr decoder;
        if (const auto res = CreateDecoder(amfContext, codec, kPlatformMemoryType, &context, &decoder); res != AMF_OK) {
            return false;
        }
        return true;
    }

    AMF_RESULT AMDDecoder::CreateDecoder(
        rtc::scoped_refptr<AMDContext> amfContext,
        const webrtc::VideoCodecType codec,
        const amf::AMF_MEMORY_TYPE memoryType,
        amf::AMFContext** outContext,
        amf::AMFComponent** outDecoder
    ) {
        if (!(codec == webrtc::kVideoCodecVP9 || codec == webrtc::kVideoCodecH264 || codec == webrtc::kVideoCodecH265 || codec == webrtc::kVideoCodecAV1)) {
            return AMF_NOT_SUPPORTED;
        }

        AMF_RESULT res = AMF_OK;

        amf::AMFContextPtr context;
        amf::AMFComponentPtr decoder;
        res = GetAMFFactoryHelper(amfContext)->GetFactory()->CreateContext(&context);
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
            res = context->InitDX9(nullptr);
            RETURN_IF_FAILED(res, "Failed to InitDX9(NULL)");
        } else if (memoryType == amf::AMF_MEMORY_DX11) {
            res = context->InitDX11(nullptr);
            RETURN_IF_FAILED(res, "Failed to InitDX11(NULL)");
        } else if (memoryType == amf::AMF_MEMORY_DX12) {
            res = amf::AMFContext2Ptr(context)->InitDX12(nullptr);
            RETURN_IF_FAILED(res, "Failed to InitDX12(NULL)");
        }
#endif

        const auto codecName =
            codec == webrtc::kVideoCodecVP9    ? AMFVideoDecoderHW_VP9
            : codec == webrtc::kVideoCodecH264 ? AMFVideoDecoderUVD_H264_AVC
            : codec == webrtc::kVideoCodecH265 ? AMFVideoDecoderHW_H265_HEVC
            : AMFVideoDecoderHW_AV1;
        res = GetAMFFactoryHelper(amfContext)->GetFactory()->CreateComponent(context, codecName, &decoder);
        RETURN_IF_FAILED(res, L"CreateComponent() failed");

        res = decoder->Init(amf::AMF_SURFACE_YUV420P, 4096, 4096);
        RETURN_IF_FAILED(res, L"Init() failed");

        *outContext = context.Detach();
        *outDecoder = decoder.Detach();

        return AMF_OK;
    }

    AMF_RESULT AMDDecoder::InitAMF() {
        memoryType = kPlatformMemoryType;

        const auto encoderRes = CreateDecoder(
            AMDContext::GetOrCreateDefault(),
            codec,
            memoryType,
            &context,
            &decoder
        );
        RETURN_IF_FAILED(encoderRes, "Failed to CreateDecoder()");

        pollingThread = std::make_unique<std::thread>([this] {
            AMF_RESULT res = AMF_OK;
            while (true) {
                amf::AMFDataPtr data;
                res = decoder->QueryOutput(&data);
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
                amf::AMFSurfacePtr surface(data);
                ProcessSurface(surface);
            }
        });
        return encoderRes;
    }

    bool AMDDecoder::Configure(const Settings& settings) {
        if (InitAMF() != AMF_OK) {
            return false;
        }
        return true;
    }

    int32_t AMDDecoder::Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs) {
        if (decoder == nullptr) {
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        if (callbackDecodedImage == nullptr) {
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        if (inputImage.data() == nullptr && inputImage.size() > 0) {
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }

        amf::AMFBufferPtr buffer;
        auto res = context->AllocBuffer(amf::AMF_MEMORY_HOST, inputImage.size(), &buffer);
        RETURN_IF_FAILED(res, "Failed to AllocBuffer()");

        memcpy(buffer->GetNative(), inputImage.data(), inputImage.size());
        buffer->SetPts(inputImage.RtpTimestamp());
        while (true) {
            if (res == AMF_REPEAT) {
                res = decoder->SubmitInput(nullptr);
            } else {
                res = decoder->SubmitInput(buffer);
            }
            if (res == AMF_NEED_MORE_INPUT) {
                return WEBRTC_VIDEO_CODEC_OK;
            }
            if (res == AMF_INPUT_FULL || res == AMF_DECODER_NO_FREE_SURFACES ||
                res == AMF_REPEAT) {
                amf_sleep(1);
            } else {
                RETURN_IF_FAILED(res, L"Failed to SubmitInput()");
                break;
            }
        }

        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t AMDDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) {
        callbackDecodedImage = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    void AMDDecoder::ReleaseAMF() {
        if (decoder != nullptr) {
            decoder->Drain();
        }
        if (pollingThread != nullptr) {
            pollingThread->join();
        }
        decoder = nullptr;
        pollingThread.reset();
    }

    AMF_RESULT AMDDecoder::ProcessSurface(const amf::AMFSurfacePtr& surface) {
        AMF_RESULT res = AMF_OK;
        res = surface->Convert(amf::AMF_MEMORY_HOST);
        RETURN_IF_FAILED(res, "Failed to Convert()");

        uint32_t pts = surface->GetPts();
        RTC_LOG(LS_ERROR) << "pts=" << pts;

        const auto py = surface->GetPlane(amf::AMF_PLANE_Y);
        const auto pu = surface->GetPlane(amf::AMF_PLANE_U);
        const auto pv = surface->GetPlane(amf::AMF_PLANE_V);

        rtc::scoped_refptr<webrtc::I420Buffer> i420Buffer = bufferPool.CreateI420Buffer(py->GetWidth(), py->GetHeight());
        libyuv::I420Copy(
            static_cast<const uint8_t*>(py->GetNative()),
            py->GetHPitch(),
            static_cast<const uint8_t*>(pu->GetNative()),
            pu->GetHPitch(),
            static_cast<const uint8_t*>(pv->GetNative()),
            pv->GetHPitch(),
            i420Buffer->MutableDataY(),
            i420Buffer->StrideY(),
            i420Buffer->MutableDataU(),
            i420Buffer->StrideU(),
            i420Buffer->MutableDataV(),
            i420Buffer->StrideV(),
            py->GetWidth(), py->GetHeight()
        );

        webrtc::VideoFrame decodedImage = webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(i420Buffer)
            .set_timestamp_rtp(pts)
            .build();
        callbackDecodedImage->Decoded(decodedImage, std::nullopt, std::nullopt);
        return res;
    }

    int32_t AMDDecoder::Release() {
        ReleaseAMF();
        bufferPool.Release();
        return WEBRTC_VIDEO_CODEC_OK;
    }

    const char* AMDDecoder::ImplementationName() const {
        return "AMF";
    }

    std::unique_ptr<AMDDecoder> AMDDecoder::Create(webrtc::VideoCodecType codec) {
        return std::make_unique<AMDDecoder>(codec);
    }
} // amd