//
// Created by Lauren on 20/11/26.
//

#include <ranges>
#include <api/video/encoded_image.h>
#include <api/video/i420_buffer.h>
#include <libyuv/planar_functions.h>
#include <media/base/media_constants.h>
#include <modules/video_coding/include/video_codec_interface.h>
#include <modules/video_coding/include/video_error_codes.h>
#include <ntgcalls/media/mixers/video_mixer.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>
#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>

namespace ntgcalls::media::mixers {
    VideoMixer::VideoMixer(BaseSink* sink, CodecFilter supports, PacketCallback packet_callback): Mixer(sink, std::move(supports), std::move(packet_callback)), raw_(supports_(wrtc::models::Codec::I420)) {
        if (const auto video_sink = dynamic_cast<VideoSink*>(sink)) {
            if (const auto description = video_sink->get_config()) {
                composer_ = std::make_unique<composer::VideoComposer>(description->width, description->height);
            }
        }
    }

    VideoMixer::~VideoMixer() {
        const std::lock_guard lock(state_mutex_);
        for (const auto& decoder : decoders_ | std::views::values) {
            decoder->Release();
        }
        if (encoder_) {
            encoder_->Release();
        }
    }

    void VideoMixer::send_frames(std::vector<wrtc::models::Frame> frames) {
        if (raw_) {
            send_raw(frames);
            return;
        }
        const std::lock_guard lock(state_mutex_);
        for (const auto& frame : frames) {
            if (!composing_) {
                if (!source_ssrc_) {
                    source_ssrc_ = frame.ssrc;
                }
                if (*source_ssrc_ != frame.ssrc || !supports_(frame.frame_data.codec)) {
                    RTC_LOG(LS_INFO) << "Composing the video recording, which re-encodes it";
                    composing_ = true;
                }
            }
            if (composing_) {
                decode(frame);
            } else {
                forward(frame);
            }
        }
    }

    void VideoMixer::remove_source(const int64_t ssrc) {
        if (composer_) {
            composer_->remove(ssrc);
        }
    }

    void VideoMixer::forward(const wrtc::models::Frame& frame) {
        if (!track_) {
            codec_ = frame.frame_data.codec;
            track_ = containers::TrackInfo{*codec_, 0, frame.frame_data.width, frame.frame_data.height};
        }
        write_frame(frame.data.data(), frame.data.size(), frame.frame_data.absolute_capture_timestamp_ms, frame.frame_data.keyframe);
    }

    void VideoMixer::decode(const wrtc::models::Frame& frame) {
        if (!composer_ || !ensure_decoder(frame.ssrc, frame.frame_data.codec)) {
            return;
        }
        if (!frame.frame_data.keyframe && !synchronized_.contains(frame.ssrc)) {
            return;
        }
        synchronized_.insert(frame.ssrc);
        webrtc::EncodedImage image;
        image.SetEncodedData(webrtc::EncodedImageBuffer::Create(frame.data.data(), frame.data.size()));
        image._encodedWidth = frame.frame_data.width;
        image._encodedHeight = frame.frame_data.height;
        image._frameType = frame.frame_data.keyframe ? webrtc::VideoFrameType::kVideoFrameKey : webrtc::VideoFrameType::kVideoFrameDelta;
        decode_timestamp_ += 3000;
        image.SetRtpTimestamp(decode_timestamp_);
        decoding_ssrc_ = frame.ssrc;
        decoders_[frame.ssrc]->Decode(image, 0);
    }

    bool VideoMixer::ensure_decoder(const int64_t ssrc, const wrtc::models::Codec codec) {
        if (const auto it = decoder_codecs_.find(ssrc); it != decoder_codecs_.end() && it->second == codec) {
            return decoders_[ssrc] != nullptr;
        }
        if (const auto it = decoders_.find(ssrc); it != decoders_.end() && it->second) {
            it->second->Release();
        }
        synchronized_.erase(ssrc);
        const auto factory = wrtc::interfaces::peer_connection::PeerConnectionFactory::get_or_create_default()->video_decoder_factory()->inner();
        auto decoder = factory->Create(wrtc::interfaces::peer_connection::PeerConnectionFactory::environment(), sdp_format(codec));
        decoder_codecs_[ssrc] = codec;
        if (!decoder) {
            RTC_LOG(LS_ERROR) << "No decoder available for the recorded video";
            decoders_[ssrc] = nullptr;
            return false;
        }
        webrtc::VideoDecoder::Settings settings;
        settings.set_codec_type(codec_type(codec));
        settings.set_number_of_cores(1);
        if (!decoder->Configure(settings)) {
            decoders_[ssrc] = nullptr;
            return false;
        }
        decoder->RegisterDecodeCompleteCallback(this);
        decoders_[ssrc] = std::move(decoder);
        return true;
    }

    int32_t VideoMixer::Decoded(webrtc::VideoFrame& decoded_image) {
        const auto buffer = decoded_image.video_frame_buffer()->ToI420();
        const auto width = buffer->width();
        const auto height = buffer->height();
        const auto chroma_width = (width + 1) / 2;
        const auto chroma_height = (height + 1) / 2;
        const auto luma_size = static_cast<size_t>(width) * height;
        const auto chroma_size = static_cast<size_t>(chroma_width) * chroma_height;
        bytes::binary frame(luma_size + chroma_size * 2);
        libyuv::I420Copy(
            buffer->DataY(), buffer->StrideY(),
            buffer->DataU(), buffer->StrideU(),
            buffer->DataV(), buffer->StrideV(),
            frame.data(), width,
            frame.data() + luma_size, chroma_width,
            frame.data() + luma_size + chroma_size, chroma_width,
            width, height
        );
        composer_->update(decoding_ssrc_, std::move(frame), static_cast<uint16_t>(width), static_cast<uint16_t>(height), decoded_image.rotation());
        return WEBRTC_VIDEO_CODEC_OK;
    }

    void VideoMixer::send_raw(std::vector<wrtc::models::Frame>& frames) const {
        if (!composer_) {
            return;
        }
        for (auto& frame : frames) {
            composer_->update(frame.ssrc, std::move(frame.data), frame.frame_data.width, frame.frame_data.height, frame.frame_data.rotation);
        }
    }

    void VideoMixer::write_raw() const {
        if (!composer_) {
            return;
        }
        packet_callback_(
            {wrtc::models::Codec::I420, 0, 0, 0},
            {composer_->compose(), composer_->frame_size(), 0, std::chrono::duration_cast<std::chrono::microseconds>(sink_->frame_time()), true}
        );
    }

    void VideoMixer::tick() {
        if (raw_) {
            write_raw();
            return;
        }
        const std::lock_guard lock(state_mutex_);
        if (!composing_ || !composer_ || !ensure_encoder()) {
            return;
        }
        const auto description = dynamic_cast<VideoSink*>(sink_)->get_config();
        const int width = description->width & ~1;
        const int height = description->height & ~1;
        const auto composed = composer_->compose();
        const auto luma_size = static_cast<size_t>(width) * height;
        const auto buffer = webrtc::I420Buffer::Copy(
            width, height,
            composed.get(), width,
            composed.get() + luma_size, width / 2,
            composed.get() + luma_size + luma_size / 4, width / 2
        );
        encode_timestamp_ += 90000 / std::max<int>(description->fps, 1);
        const auto frame = webrtc::VideoFrame::Builder()
                               .set_video_frame_buffer(buffer)
                               .set_rtp_timestamp(encode_timestamp_)
                               .set_timestamp_us(webrtc::TimeMicros())
                               .build();
        const auto now = std::chrono::steady_clock::now();
        const bool keyframe = !last_keyframe_ || now - *last_keyframe_ >= kKeyframeInterval;
        if (keyframe) {
            last_keyframe_ = now;
        }
        const std::vector types{keyframe ? webrtc::VideoFrameType::kVideoFrameKey : webrtc::VideoFrameType::kVideoFrameDelta};
        encoder_->Encode(frame, &types);
    }

    wrtc::models::Codec VideoMixer::encoding_codec() const {
        for (const auto codec : {wrtc::models::Codec::VP8, wrtc::models::Codec::H264, wrtc::models::Codec::VP9, wrtc::models::Codec::AV1}) {
            if (supports_(codec)) {
                return codec;
            }
        }
        return wrtc::models::Codec::VP8;
    }

    bool VideoMixer::ensure_encoder() {
        if (encoder_) {
            return true;
        }
        const auto description = dynamic_cast<VideoSink*>(sink_)->get_config();
        if (!description) {
            return false;
        }
        const auto codec = codec_.value_or(encoding_codec());
        const auto factory = wrtc::interfaces::peer_connection::PeerConnectionFactory::get_or_create_default()->video_encoder_factory();
        encoder_ = factory->Create(wrtc::interfaces::peer_connection::PeerConnectionFactory::environment(), sdp_format(codec));
        if (!encoder_) {
            RTC_LOG(LS_ERROR) << "No encoder available for the composed video";
            return false;
        }
        webrtc::VideoCodec settings{};
        settings.codecType = codec_type(codec);
        settings.width = description->width & ~1;
        settings.height = description->height & ~1;
        settings.maxFramerate = std::max<int>(description->fps, 1);
        settings.startBitrate = kComposedBitrate / 1000;
        settings.maxBitrate = kComposedBitrate / 1000;
        settings.minBitrate = kComposedBitrate / 4000;
        settings.qpMax = 56;
        settings.numberOfSimulcastStreams = 0;
        settings.SetFrameDropEnabled(false);
        if (codec == wrtc::models::Codec::VP8) {
            *settings.VP8() = webrtc::VideoEncoder::GetDefaultVp8Settings();
        } else if (codec == wrtc::models::Codec::VP9) {
            *settings.VP9() = webrtc::VideoEncoder::GetDefaultVp9Settings();
        } else {
            *settings.H264() = webrtc::VideoEncoder::GetDefaultH264Settings();
        }
        encoder_->RegisterEncodeCompleteCallback(this);
        if (encoder_->InitEncode(&settings, webrtc::VideoEncoder::Settings(webrtc::VideoEncoder::Capabilities(false), 1, 1200)) != WEBRTC_VIDEO_CODEC_OK) {
            RTC_LOG(LS_ERROR) << "Unable to initialize the composed video encoder";
            encoder_ = nullptr;
            return false;
        }
        webrtc::VideoBitrateAllocation allocation;
        allocation.SetBitrate(0, 0, kComposedBitrate);
        encoder_->SetRates(webrtc::VideoEncoder::RateControlParameters(allocation, settings.maxFramerate));
        if (!track_) {
            codec_ = codec;
            track_ = containers::TrackInfo{codec, 0, settings.width, settings.height};
        }
        return true;
    }

    webrtc::EncodedImageCallback::Result VideoMixer::OnEncodedImage(const webrtc::EncodedImage& encoded_image, const webrtc::CodecSpecificInfo*) {
        write_frame(encoded_image.data(), encoded_image.size(), webrtc::TimeMillis(), encoded_image._frameType == webrtc::VideoFrameType::kVideoFrameKey);
        return Result(Result::OK);
    }

    void VideoMixer::OnFrameDropped(uint32_t, int, bool) {}

    void VideoMixer::write_frame(const uint8_t* data, const size_t size, const int64_t timestamp_ms, const bool keyframe) const {
        packet_callback_(*track_, {bytes::make_unique_binary(data, size), size, timestamp_ms, std::chrono::microseconds(0), keyframe});
    }

    std::chrono::nanoseconds VideoMixer::tick_interval() const {
        return sink_->frame_time();
    }

    webrtc::SdpVideoFormat VideoMixer::sdp_format(const wrtc::models::Codec codec) {
        switch (codec) {
        case wrtc::models::Codec::VP8:
            return webrtc::SdpVideoFormat(webrtc::kVp8CodecName);
        case wrtc::models::Codec::VP9:
            return webrtc::SdpVideoFormat(webrtc::kVp9CodecName);
        case wrtc::models::Codec::AV1:
            return webrtc::SdpVideoFormat(webrtc::kAv1CodecName);
        default:
            return webrtc::SdpVideoFormat(webrtc::kH264CodecName, {{"profile-level-id", "42e01f"}, {"packetization-mode", "1"}, {"level-asymmetry-allowed", "1"}});
        }
    }

    webrtc::VideoCodecType VideoMixer::codec_type(const wrtc::models::Codec codec) {
        switch (codec) {
        case wrtc::models::Codec::VP8:
            return webrtc::kVideoCodecVP8;
        case wrtc::models::Codec::VP9:
            return webrtc::kVideoCodecVP9;
        case wrtc::models::Codec::AV1:
            return webrtc::kVideoCodecAV1;
        default:
            return webrtc::kVideoCodecH264;
        }
    }
} // ntgcalls::media::mixers
