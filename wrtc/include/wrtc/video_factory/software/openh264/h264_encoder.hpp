//
// Created by Laky64 on 04/11/24.
//

#pragma once

#ifndef IS_ANDROID
#include <wels/codec_api.h>
#include <api/environment/environment.h>
#include <api/video/i420_buffer.h>
#include <api/video_codecs/video_encoder.h>
#include <common_video/h264/h264_bitstream_parser.h>
#include <media/base/codec.h>
#include <modules/video_coding/svc/scalable_video_controller.h>
#include <wrtc/video_factory/software/openh264/layer_config.hpp>
#include <modules/video_coding/codecs/h264/include/h264_globals.h>

namespace openh264 {

    class H264Encoder final : public webrtc::VideoEncoder {
        enum H264EncoderImplEvent {
            kH264EncoderEventInit = 0,
            kH264EncoderEventError = 1,
            kH264EncoderEventMax = 16,
        };

        static constexpr int kLowH264QpThreshold = 24;
        static constexpr int kHighH264QpThreshold = 37;

        bool hasReportedError;
        bool hasReportedInit;
        size_t maxPayloadSize;
        int32_t numberOfCores;
        std::optional<int> encoderThreadLimit;
        webrtc::VideoCodec codec;
        const webrtc::Environment env;
        std::vector<uint8_t> tl0syncLimit;
        std::vector<ISVCEncoder*> encoders;
        std::vector<SSourcePicture> pictures;
        std::vector<webrtc::EncodedImage> encodedImages;
        webrtc::H264PacketizationMode packetizationMode;
        webrtc::H264BitstreamParser h264BitstreamParser;
        std::vector<rtc::scoped_refptr<webrtc::I420Buffer>> downscaledBuffers;
        std::vector<std::unique_ptr<webrtc::ScalableVideoController>> svcControllers;
        std::vector<LayerConfig> configurations;
        webrtc::EncodedImageCallback* encodedImageCallback;
        absl::InlinedVector<std::optional<webrtc::ScalabilityMode>, webrtc::kMaxSimulcastStreams> scalabilityModes;

        void ReportError();

        static webrtc::VideoFrameType ConvertToVideoFrameType(EVideoFrameType type);

        static void RtpFragmentize(webrtc::EncodedImage* encodedImage, SFrameBSInfo* info);

        void ReportInit();

        static std::optional<webrtc::ScalabilityMode> ScalabilityModeFromTemporalLayers(int numTemporalLayers);

        [[nodiscard]] SEncParamExt CreateEncoderParams(size_t i) const;

        static int NumberOfThreads(std::optional<int> encoderThreadLimit, int width, int height, int numberOfCores);

    public:
        explicit H264Encoder(webrtc::Environment env);

        ~H264Encoder() override;

        int32_t InitEncode(const webrtc::VideoCodec* inst, const Settings& settings) override;

        int32_t RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) override;

        int32_t Release() override;

        int32_t Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frame_types) override;

        void SetRates(const RateControlParameters& parameters) override;

        [[nodiscard]] EncoderInfo GetEncoderInfo() const override;
    };

} // openh264
#endif