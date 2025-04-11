//
// Created by Laky64 on 11/04/25.
//

#pragma once

#include <thread>
#include <public/common/TraceAdapter.h>
#include <api/video_codecs/video_decoder.h>
#include <common_video/include/video_frame_buffer_pool.h>
#include <wrtc/video_factory/hardware/amd/amd_context.hpp>

namespace amd {

    class AMDDecoder final: public webrtc::VideoDecoder {
        std::unique_ptr<std::thread> pollingThread;
        amf::AMFContextPtr context;
        amf::AMFComponentPtr decoder;
        amf::AMF_MEMORY_TYPE memoryType = amf::AMF_MEMORY_UNKNOWN;

        webrtc::VideoCodecType codec;
        webrtc::VideoFrameBufferPool bufferPool;
        webrtc::DecodedImageCallback* callbackDecodedImage = nullptr;

        AMF_RESULT InitAMF();

        void ReleaseAMF();

        AMF_RESULT ProcessSurface(const amf::AMFSurfacePtr& surface);

    public:
        explicit AMDDecoder(webrtc::VideoCodecType codec);

        ~AMDDecoder() override;

        static bool IsSupported(webrtc::VideoCodecType codec);

        static std::unique_ptr<AMDDecoder> Create(
            webrtc::VideoCodecType codec
        );

        static AMF_RESULT CreateDecoder(
            rtc::scoped_refptr<AMDContext> amfContext,
            webrtc::VideoCodecType codec,
            amf::AMF_MEMORY_TYPE memoryType,
            amf::AMFContext** outContext,
            amf::AMFComponent** outDecoder
        );

        bool Configure(const Settings& settings) override;

        int32_t Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs) override;

        int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) override;

        int32_t Release() override;

        const char* ImplementationName() const override;
    };

} // amd
