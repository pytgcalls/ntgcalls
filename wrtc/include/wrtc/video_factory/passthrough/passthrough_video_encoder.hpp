//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <api/environment/environment.h>
#include <api/video_codecs/sdp_video_format.h>
#include <api/video_codecs/video_decoder.h>
#include <api/video_codecs/video_decoder_factory.h>
#include <api/video_codecs/video_encoder.h>
#include <wrtc/utils/encoded/encoded_queue.hpp>

namespace wrtc::video_factory {
    class PassthroughVideoEncoder final: public webrtc::VideoEncoder, public webrtc::DecodedImageCallback {
        static constexpr size_t kMaxPendingFrames = 900;
        static constexpr double kLowBitrateRatio = 0.8;
        static constexpr double kRecoveredBitrateRatio = 1.1;
        static constexpr int64_t kBitrateWindowUs = 2000000;
        static constexpr int64_t kBitrateHoldUs = 5000000;

        enum class Mode {
            Forward,
            Encode
        };

        webrtc::Environment env_;
        std::shared_ptr<utils::encoded::EncodedQueue> queue_;
        std::unique_ptr<VideoEncoder> encoder_;
        webrtc::VideoDecoderFactory* decoder_factory_;
        std::unique_ptr<webrtc::VideoDecoder> decoder_;
        std::optional<models::Codec> decoder_codec_;
        webrtc::SdpVideoFormat format_;
        models::Codec codec_;
        webrtc::VideoCodec codec_settings_{};
        std::optional<Settings> settings_;
        std::optional<RateControlParameters> rates_;
        webrtc::EncodedImageCallback* callback_ = nullptr;
        bool encoder_initialized_ = false;
        Mode mode_ = Mode::Forward;
        std::vector<utils::encoded::EncodedFrame> pending_;
        std::optional<webrtc::VideoFrame> decoded_;
        uint32_t decode_timestamp_ = 0;
        int64_t window_start_us_ = -1;
        size_t window_bytes_ = 0;
        double source_bps_ = 0;
        int64_t bitrate_change_us_ = -1;
        bool bitrate_low_ = false;
        bool reordered_ = false;
        bool forwarded_ = false;
        int64_t last_frame_us_ = 0;

        [[nodiscard]] bool compatible(const utils::encoded::EncodedFrame& frame) const;

        void detect_reordering(const utils::encoded::EncodedFrame& frame);

        [[nodiscard]] bool mismatched(const utils::encoded::EncodedFrame& frame) const;

        [[nodiscard]] std::string incompatibility(const utils::encoded::EncodedFrame& frame, bool key_requested) const;

        static bool has_b_slices(const bytes::binary& payload);

        void forward(const utils::encoded::EncodedFrame& frame, const webrtc::VideoFrame& input, uint32_t rtp_timestamp) const;

        bool encode(const webrtc::VideoFrame& input, uint32_t rtp_timestamp, bool keyframe);

        bool ensure_decoder(models::Codec codec);

        bool ensure_encoder();

        void decode(const utils::encoded::EncodedFrame& frame);

        void track_bitrate(size_t bytes, int64_t timestamp_us);

        void update_bitrate_state(double available_bps, int64_t timestamp_us);

        [[nodiscard]] size_t top_layer() const;

        static models::Codec codec_from_name(const std::string& name);

        static webrtc::VideoCodecType codec_type(models::Codec codec);

        static std::string codec_name(models::Codec codec);

        static webrtc::CodecSpecificInfo codec_specific_info(models::Codec codec, bool keyframe, uint16_t width, uint16_t height);

    public:
        PassthroughVideoEncoder(webrtc::Environment  env, std::shared_ptr<utils::encoded::EncodedQueue> queue, std::unique_ptr<VideoEncoder> encoder, webrtc::VideoDecoderFactory* decoder_factory, webrtc::SdpVideoFormat format);

        ~PassthroughVideoEncoder() override;

        int InitEncode(const webrtc::VideoCodec* codec_settings, const Settings& settings) override;

        int32_t RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) override;

        int32_t Release() override;

        int32_t Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frame_types) override;

        void SetRates(const RateControlParameters& parameters) override;

        void OnPacketLossRateUpdate(float packet_loss_rate) override;

        void OnRttUpdate(int64_t rtt_ms) override;

        void OnLossNotification(const LossNotification& loss_notification) override;

        [[nodiscard]] EncoderInfo GetEncoderInfo() const override;

        int32_t Decoded(webrtc::VideoFrame& decoded_image) override;
    };
} // wrtc::video_factory
