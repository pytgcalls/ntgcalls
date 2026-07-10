//
// Created by Lauren on 04/11/24.
//

#pragma once

#ifndef IS_ANDROID
#include <api/environment/environment.h>
#include <api/video/i420_buffer.h>
#include <api/video_codecs/video_encoder.h>
#include <common_video/h264/h264_bitstream_parser.h>
#include <media/base/codec.h>
#include <modules/video_coding/codecs/h264/include/h264_globals.h>
#include <modules/video_coding/svc/scalable_video_controller.h>
#include <wels/codec_api.h>
#include <wrtc/video_factory/software/openh264/layer_config.hpp>

namespace openh264 {

    class H264Encoder final : public webrtc::VideoEncoder {
        enum H264EncoderImplEvent {
            H264EncoderEventInit = 0,
            H264EncoderEventError = 1,
            H264EncoderEventMax = 16,
        };

        static constexpr int kLowH264QpThreshold = 24;
        static constexpr int kHighH264QpThreshold = 37;

        bool has_reported_error_;
        bool has_reported_init_;
        size_t max_payload_size_;
        int32_t number_of_cores_;
        std::optional<int> encoder_thread_limit_;
        webrtc::VideoCodec codec_;
        const webrtc::Environment env_;
        std::vector<uint8_t> tl0sync_limit_;
        std::vector<ISVCEncoder*> encoders_;
        std::vector<SSourcePicture> pictures_;
        std::vector<webrtc::EncodedImage> encoded_images_;
        webrtc::H264PacketizationMode packetization_mode_;
        webrtc::H264BitstreamParser h264_bitstream_parser_;
        std::vector<webrtc::scoped_refptr<webrtc::I420Buffer>> downscaled_buffers_;
        std::vector<std::unique_ptr<webrtc::ScalableVideoController>> svc_controllers_;
        std::vector<LayerConfig> configurations_;
        webrtc::EncodedImageCallback* encoded_image_callback_;
        absl::InlinedVector<std::optional<webrtc::ScalabilityMode>, webrtc::kMaxSimulcastStreams> scalability_modes_;

        void report_error();

        static webrtc::VideoFrameType convert_to_video_frame_type(EVideoFrameType type);

        static void rtp_fragmentize(webrtc::EncodedImage* encoded_image, SFrameBSInfo* info);

        void report_init();

        static std::optional<webrtc::ScalabilityMode> scalability_mode_from_temporal_layers(int num_temporal_layers);

        [[nodiscard]] SEncParamExt create_encoder_params(size_t i) const;

        static int number_of_threads(std::optional<int> encoder_thread_limit, int width, int height, int number_of_cores);

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