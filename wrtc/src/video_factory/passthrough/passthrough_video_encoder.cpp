//
// Created by Lauren on 20/11/26.
//

#include <api/video/encoded_image.h>
#include <media/base/media_constants.h>
#include <modules/video_coding/codecs/interface/common_constants.h>
#include <modules/video_coding/include/video_codec_interface.h>
#include <modules/video_coding/include/video_error_codes.h>
#include <common_video/h264/h264_common.h>
#include <rtc_base/bitstream_reader.h>
#include <rtc_base/logging.h>
#include <utility>
#include <wrtc/video_factory/passthrough/passthrough_video_encoder.hpp>

namespace wrtc::video_factory {
    PassthroughVideoEncoder::PassthroughVideoEncoder(webrtc::Environment  env, std::shared_ptr<utils::encoded::EncodedQueue> queue, std::unique_ptr<VideoEncoder> encoder, webrtc::VideoDecoderFactory* decoder_factory, webrtc::SdpVideoFormat format):
    env_(std::move(env)), queue_(std::move(queue)), encoder_(std::move(encoder)), decoder_factory_(decoder_factory), format_(std::move(format)), codec_(codec_from_name(format_.name)) {}

    PassthroughVideoEncoder::~PassthroughVideoEncoder() {
        PassthroughVideoEncoder::Release();
    }

    int PassthroughVideoEncoder::InitEncode(const webrtc::VideoCodec* codec_settings, const Settings& settings) {
        codec_settings_ = *codec_settings;
        settings_ = settings;
        if (encoder_initialized_ && encoder_->InitEncode(&codec_settings_, settings) != WEBRTC_VIDEO_CODEC_OK) {
            encoder_initialized_ = false;
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t PassthroughVideoEncoder::RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) {
        callback_ = callback;
        if (encoder_initialized_) {
            encoder_->RegisterEncodeCompleteCallback(callback);
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t PassthroughVideoEncoder::Release() {
        if (encoder_initialized_) {
            encoder_->Release();
            encoder_initialized_ = false;
        }
        if (decoder_) {
            decoder_->Release();
            decoder_ = nullptr;
            decoder_codec_ = std::nullopt;
        }
        decoded_ = std::nullopt;
        pending_.clear();
        mode_ = Mode::Forward;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t PassthroughVideoEncoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frame_types) {
        if (!callback_) {
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        bool key_requested = false;
        if (frame_types) {
            for (const auto type : *frame_types) {
                key_requested |= type == webrtc::VideoFrameType::kVideoFrameKey;
            }
        }
        std::vector<utils::encoded::EncodedFrame> frames;
        while (auto encoded = queue_->pop()) {
            frames.push_back(std::move(*encoded));
        }
        for (size_t i = 0; i < frames.size(); i++) {
            auto& encoded = frames[i];
            detect_reordering(encoded);
            const auto rtp_timestamp = frame.rtp_timestamp() - static_cast<uint32_t>(frames.size() - 1 - i);
            const bool keyframe = encoded.frame_data.keyframe;
            track_bitrate(encoded.payload.size(), frame.timestamp_us());
            if (keyframe) {
                pending_.clear();
            }
            if (mode_ == Mode::Encode && keyframe && compatible(encoded)) {
                mode_ = Mode::Forward;
                RTC_LOG(LS_INFO) << "Source keyframe reached, forwarding the encoded video again";
            }
            if (mode_ == Mode::Forward) {
                if (compatible(encoded) && (!key_requested || keyframe)) {
                    if (!forwarded_) {
                        RTC_LOG(LS_INFO) << "Forwarding the encoded " << codec_name(codec_) << " video without re-encoding it";
                        forwarded_ = true;
                    }
                    forward(encoded, frame, rtp_timestamp);
                    key_requested = false;
                    if (pending_.size() < kMaxPendingFrames) {
                        pending_.push_back(std::move(encoded));
                    }
                    continue;
                }
                pending_.push_back(std::move(encoded));
                if (!ensure_decoder(pending_.back().frame_data.codec) || !ensure_encoder()) {
                    continue;
                }
                if (mismatched(pending_.back())) {
                    RTC_LOG(LS_INFO) << "Re-encoding the video, " << incompatibility(pending_.back(), key_requested);
                } else {
                    RTC_LOG(LS_INFO) << "Encoding the video until the next usable source keyframe, " << incompatibility(pending_.back(), key_requested);
                }
                for (const auto& pending : pending_) {
                    decode(pending);
                }
                pending_.clear();
                mode_ = Mode::Encode;
                encode(frame, rtp_timestamp, true);
                key_requested = false;
                continue;
            }
            if (!ensure_decoder(encoded.frame_data.codec)) {
                continue;
            }
            decode(encoded);
            encode(frame, rtp_timestamp, key_requested);
            key_requested = false;
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }

    bool PassthroughVideoEncoder::compatible(const utils::encoded::EncodedFrame& frame) const {
        return !bitrate_low_ && !reordered_ && frame.frame_data.codec == codec_ && frame.frame_data.width == codec_settings_.width && frame.frame_data.height == codec_settings_.height;
    }

    void PassthroughVideoEncoder::forward(const utils::encoded::EncodedFrame& frame, const webrtc::VideoFrame& input, const uint32_t rtp_timestamp) const {
        const bool keyframe = frame.frame_data.keyframe;
        webrtc::EncodedImage image;
        image.SetEncodedData(webrtc::EncodedImageBuffer::Create(frame.payload.data(), frame.payload.size()));
        image._encodedWidth = frame.frame_data.width;
        image._encodedHeight = frame.frame_data.height;
        image._frameType = keyframe ? webrtc::VideoFrameType::kVideoFrameKey : webrtc::VideoFrameType::kVideoFrameDelta;
        image.SetRtpTimestamp(rtp_timestamp);
        image.capture_time_ms_ = input.render_time_ms();
        image.rotation_ = input.rotation();
        image.SetSimulcastIndex(static_cast<int>(top_layer()));
        const auto info = codec_specific_info(codec_, keyframe, frame.frame_data.width, frame.frame_data.height);
        callback_->OnEncodedImage(image, &info);
    }

    bool PassthroughVideoEncoder::encode(const webrtc::VideoFrame& input, const uint32_t rtp_timestamp, const bool keyframe) {
        if (!decoded_) {
            return false;
        }
        const auto frame = webrtc::VideoFrame::Builder()
                               .set_video_frame_buffer(decoded_->video_frame_buffer())
                               .set_rtp_timestamp(rtp_timestamp)
                               .set_timestamp_us(input.timestamp_us())
                               .set_rotation(input.rotation())
                               .build();
        const std::vector types(std::max<size_t>(codec_settings_.numberOfSimulcastStreams, 1), keyframe ? webrtc::VideoFrameType::kVideoFrameKey : webrtc::VideoFrameType::kVideoFrameDelta);
        return encoder_->Encode(frame, &types) == WEBRTC_VIDEO_CODEC_OK;
    }

    bool PassthroughVideoEncoder::ensure_decoder(const models::Codec codec) {
        if (decoder_ && decoder_codec_ == codec) {
            return true;
        }
        if (decoder_) {
            decoder_->Release();
        }
        decoder_ = decoder_factory_->Create(env_, webrtc::SdpVideoFormat(codec_name(codec)));
        decoder_codec_ = codec;
        if (!decoder_) {
            RTC_LOG(LS_ERROR) << "No decoder available for the source video";
            return false;
        }
        webrtc::VideoDecoder::Settings settings;
        settings.set_codec_type(codec_type(codec));
        settings.set_number_of_cores(1);
        settings.set_max_render_resolution({codec_settings_.width, codec_settings_.height});
        if (!decoder_->Configure(settings)) {
            decoder_ = nullptr;
            return false;
        }
        decoder_->RegisterDecodeCompleteCallback(this);
        return true;
    }

    bool PassthroughVideoEncoder::ensure_encoder() {
        if (encoder_initialized_) {
            return true;
        }
        if (!settings_) {
            return false;
        }
        encoder_->RegisterEncodeCompleteCallback(callback_);
        if (encoder_->InitEncode(&codec_settings_, *settings_) != WEBRTC_VIDEO_CODEC_OK) {
            RTC_LOG(LS_ERROR) << "Unable to initialize the fallback video encoder";
            return false;
        }
        if (rates_) {
            encoder_->SetRates(*rates_);
        }
        encoder_initialized_ = true;
        return true;
    }

    void PassthroughVideoEncoder::detect_reordering(const utils::encoded::EncodedFrame& frame) {
        if (!reordered_ && frame.frame_data.codec == models::Codec::H264 && has_b_slices(frame.payload)) {
            reordered_ = true;
        }
    }

    bool PassthroughVideoEncoder::mismatched(const utils::encoded::EncodedFrame& frame) const {
        return reordered_ || frame.frame_data.codec != codec_ || frame.frame_data.width != codec_settings_.width || frame.frame_data.height != codec_settings_.height;
    }

    std::string PassthroughVideoEncoder::incompatibility(const utils::encoded::EncodedFrame& frame, const bool key_requested) const {
        if (reordered_) {
            return "the source uses B-frames";
        }
        if (frame.frame_data.codec != codec_) {
            return "the source is " + codec_name(frame.frame_data.codec) + " while the call uses " + codec_name(codec_);
        }
        if (frame.frame_data.width != codec_settings_.width || frame.frame_data.height != codec_settings_.height) {
            return "the source is " + std::to_string(frame.frame_data.width) + "x" + std::to_string(frame.frame_data.height) + " while the call sends " + std::to_string(codec_settings_.width) + "x" + std::to_string(codec_settings_.height);
        }
        if (bitrate_low_) {
            return "the bandwidth is below the source bitrate";
        }
        return key_requested ? "a viewer requested a keyframe" : "the source frame can't be forwarded";
    }

    bool PassthroughVideoEncoder::has_b_slices(const bytes::binary& payload) {
        for (const auto& index : webrtc::H264::FindNaluIndices(payload)) {
            if (index.payload_size < 2 || webrtc::H264::ParseNaluType(payload[index.payload_start_offset]) != webrtc::H264::NaluType::kSlice) {
                continue;
            }
            const auto rbsp = webrtc::H264::ParseRbsp(payload.data() + index.payload_start_offset + 1, index.payload_size - 1);
            webrtc::BitstreamReader reader(rbsp);
            reader.ReadExponentialGolomb();
            if (const auto slice_type = reader.ReadExponentialGolomb(); reader.Ok() && slice_type % 5 == 1) {
                return true;
            }
        }
        return false;
    }

    void PassthroughVideoEncoder::decode(const utils::encoded::EncodedFrame& frame) {
        webrtc::EncodedImage image;
        image.SetEncodedData(webrtc::EncodedImageBuffer::Create(frame.payload.data(), frame.payload.size()));
        image._encodedWidth = frame.frame_data.width;
        image._encodedHeight = frame.frame_data.height;
        image._frameType = frame.frame_data.keyframe ? webrtc::VideoFrameType::kVideoFrameKey : webrtc::VideoFrameType::kVideoFrameDelta;
        decode_timestamp_ += 3000;
        image.SetRtpTimestamp(decode_timestamp_);
        decoder_->Decode(image, 0);
    }

    void PassthroughVideoEncoder::track_bitrate(const size_t bytes, const int64_t timestamp_us) {
        if (window_start_us_ < 0 || timestamp_us < window_start_us_) {
            window_start_us_ = timestamp_us;
            window_bytes_ = 0;
        }
        window_bytes_ += bytes;
        if (const auto elapsed = timestamp_us - window_start_us_; elapsed >= kBitrateWindowUs) {
            source_bps_ = static_cast<double>(window_bytes_) * 8 * 1000000 / static_cast<double>(elapsed);
            window_start_us_ = timestamp_us;
            window_bytes_ = 0;
        }
    }

    void PassthroughVideoEncoder::update_bitrate_state(const double available_bps, const int64_t timestamp_us) {
        if (source_bps_ <= 0) {
            return;
        }
        const bool low = available_bps < source_bps_ * kLowBitrateRatio;
        if (const bool recovered = available_bps >= source_bps_ * kRecoveredBitrateRatio; (!bitrate_low_ && low) || (bitrate_low_ && recovered)) {
            if (bitrate_change_us_ < 0) {
                bitrate_change_us_ = timestamp_us;
            } else if (timestamp_us - bitrate_change_us_ >= kBitrateHoldUs) {
                bitrate_low_ = !bitrate_low_;
                bitrate_change_us_ = -1;
                RTC_LOG(LS_INFO) << (bitrate_low_ ? "Bandwidth below the source bitrate, encoding the video" : "Bandwidth recovered, forwarding the encoded video from the next keyframe");
            }
        } else {
            bitrate_change_us_ = -1;
        }
    }

    void PassthroughVideoEncoder::SetRates(const RateControlParameters& parameters) {
        rates_ = parameters;
        if (encoder_initialized_) {
            encoder_->SetRates(parameters);
        }
        update_bitrate_state(static_cast<double>(parameters.bandwidth_allocation.bps()), env_.clock().TimeInMicroseconds());
    }

    void PassthroughVideoEncoder::OnPacketLossRateUpdate(const float packet_loss_rate) {
        encoder_->OnPacketLossRateUpdate(packet_loss_rate);
    }

    void PassthroughVideoEncoder::OnRttUpdate(const int64_t rtt_ms) {
        encoder_->OnRttUpdate(rtt_ms);
    }

    void PassthroughVideoEncoder::OnLossNotification(const LossNotification& loss_notification) {
        encoder_->OnLossNotification(loss_notification);
    }

    webrtc::VideoEncoder::EncoderInfo PassthroughVideoEncoder::GetEncoderInfo() const {
        auto info = encoder_->GetEncoderInfo();
        info.implementation_name = "PassthroughVideoEncoder";
        info.has_trusted_rate_controller = true;
        info.scaling_settings = ScalingSettings::kOff;
        info.requested_resolution_alignment = 1;
        info.apply_alignment_to_all_simulcast_layers = false;
        return info;
    }

    int32_t PassthroughVideoEncoder::Decoded(webrtc::VideoFrame& decoded_image) {
        decoded_ = decoded_image;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    size_t PassthroughVideoEncoder::top_layer() const {
        return codec_settings_.numberOfSimulcastStreams > 1 ? codec_settings_.numberOfSimulcastStreams - 1 : 0;
    }

    models::Codec PassthroughVideoEncoder::codec_from_name(const std::string& name) {
        if (name == webrtc::kVp8CodecName) {
            return models::Codec::VP8;
        }
        if (name == webrtc::kVp9CodecName) {
            return models::Codec::VP9;
        }
        if (name == webrtc::kAv1CodecName) {
            return models::Codec::AV1;
        }
        return models::Codec::H264;
    }

    webrtc::VideoCodecType PassthroughVideoEncoder::codec_type(const models::Codec codec) {
        switch (codec) {
        case models::Codec::VP8:
            return webrtc::kVideoCodecVP8;
        case models::Codec::VP9:
            return webrtc::kVideoCodecVP9;
        case models::Codec::AV1:
            return webrtc::kVideoCodecAV1;
        default:
            return webrtc::kVideoCodecH264;
        }
    }

    std::string PassthroughVideoEncoder::codec_name(const models::Codec codec) {
        switch (codec) {
        case models::Codec::VP8:
            return webrtc::kVp8CodecName;
        case models::Codec::VP9:
            return webrtc::kVp9CodecName;
        case models::Codec::AV1:
            return webrtc::kAv1CodecName;
        default:
            return webrtc::kH264CodecName;
        }
    }

    webrtc::CodecSpecificInfo PassthroughVideoEncoder::codec_specific_info(const models::Codec codec, const bool keyframe, const uint16_t width, const uint16_t height) {
        webrtc::CodecSpecificInfo info;
        info.codecType = codec_type(codec);
        info.end_of_picture = true;
        switch (codec) {
        case models::Codec::H264:
            info.codecSpecific.H264.packetization_mode = webrtc::H264PacketizationMode::NonInterleaved;
            info.codecSpecific.H264.temporal_idx = webrtc::kNoTemporalIdx;
            info.codecSpecific.H264.base_layer_sync = false;
            info.codecSpecific.H264.idr_frame = keyframe;
            break;
        case models::Codec::VP8:
            info.codecSpecific.VP8.nonReference = false;
            info.codecSpecific.VP8.temporalIdx = webrtc::kNoTemporalIdx;
            info.codecSpecific.VP8.layerSync = false;
            info.codecSpecific.VP8.keyIdx = webrtc::kNoKeyIdx;
            break;
        case models::Codec::VP9: {
            auto& vp9 = info.codecSpecific.VP9;
            vp9.first_frame_in_picture = true;
            vp9.inter_pic_predicted = !keyframe;
            vp9.flexible_mode = false;
            vp9.ss_data_available = keyframe;
            vp9.non_ref_for_inter_layer_pred = true;
            vp9.temporal_idx = webrtc::kNoTemporalIdx;
            vp9.temporal_up_switch = false;
            vp9.inter_layer_predicted = false;
            vp9.gof_idx = 0;
            vp9.num_spatial_layers = 1;
            vp9.first_active_layer = 0;
            vp9.spatial_layer_resolution_present = keyframe;
            vp9.width[0] = width;
            vp9.height[0] = height;
            if (keyframe) {
                vp9.gof.SetGofInfoVP9(webrtc::kTemporalStructureMode1);
            }
            vp9.num_ref_pics = keyframe ? 0 : 1;
            vp9.p_diff[0] = 1;
            break;
        }
        default:
            break;
        }
        return info;
    }
} // wrtc::video_factory
