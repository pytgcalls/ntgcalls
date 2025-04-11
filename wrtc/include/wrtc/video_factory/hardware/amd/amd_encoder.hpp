//
// Created by Laky64 on 10/04/25.
//

#pragma once

#include <thread>
#include <public/common/TraceAdapter.h>
#include <api/video_codecs/video_encoder.h>
#include <common_video/h264/h264_bitstream_parser.h>
#include <common_video/h265/h265_bitstream_parser.h>
#include <common_video/include/bitrate_adjuster.h>
#include <wrtc/video_factory/hardware/amd/amd_context.hpp>
#include <modules/video_coding/svc/create_scalability_structure.h>

namespace amd {

    class AMDEncoder final: public webrtc::VideoEncoder {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t targetBitrateBps = 0;
        uint32_t maxBitrateBps = 0;
        uint32_t framerate = 0;

        std::mutex mutex;
        std::unique_ptr<std::thread> pollingThread;
        amf::AMFContextPtr context;
        amf::AMFComponentPtr encoder;
        amf::AMF_MEMORY_TYPE memoryType = amf::AMF_MEMORY_UNKNOWN;
        amf::AMFSurfacePtr surface;

        bool reconfigureNeeded = false;
        webrtc::EncodedImage encodedImage;
        webrtc::H264BitstreamParser h264BitstreamParser;
        webrtc::H265BitstreamParser h265BitstreamParser;
        webrtc::VideoCodecType codec;
        webrtc::BitrateAdjuster bitrateAdjuster;
        webrtc::EncodedImageCallback* callbackEncodedImage = nullptr;
        webrtc::VideoCodecMode mode = webrtc::VideoCodecMode::kRealtimeVideo;
        std::unique_ptr<webrtc::ScalableVideoController> svcController;
        webrtc::ScalabilityMode scalabilityMode = webrtc::ScalabilityMode::kL1T1;

        AMF_RESULT InitAMF();

        AMF_RESULT ReleaseAMF();

        AMF_RESULT ProcessBuffer(const amf::AMFBufferPtr& buffer, webrtc::VideoCodecType codec);

    public:
        explicit AMDEncoder(webrtc::VideoCodecType codec);

        ~AMDEncoder() override;

        static bool IsSupported(webrtc::VideoCodecType codec);

        static std::unique_ptr<AMDEncoder> Create(
            webrtc::VideoCodecType codec
        );

        static AMF_RESULT CreateEncoder(
            rtc::scoped_refptr<AMDContext> amdContext,
            webrtc::VideoCodecType codec,
            int width,
            int height,
            int framerate,
            int targetBitrateBps,
            int maxBitrateBps,
            amf::AMF_MEMORY_TYPE memoryType,
            amf::AMFContext** outContext,
            amf::AMFComponent** outEncoder
        );

        int32_t RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) override;

        int32_t InitEncode(const webrtc::VideoCodec* codecSettings, int32_t numberOfCores, size_t maxPayloadSize) override;

        int32_t Release() override;

        int32_t Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frameTypes) override;

        void SetRates(const RateControlParameters& parameters) override;

        EncoderInfo GetEncoderInfo() const override;
    };

} // amd
