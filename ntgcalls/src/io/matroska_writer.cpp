//
// Created by Lauren on 20/11/26.
//

#include <cstring>
#include <ranges>
#include <api/video/encoded_image.h>
#include <api/video/i420_buffer.h>
#include <libyuv/planar_functions.h>
#include <media/base/media_constants.h>
#include <modules/video_coding/include/video_codec_interface.h>
#include <modules/video_coding/include/video_error_codes.h>
#include <ntgcalls/io/matroska_writer.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>
#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>

namespace ntgcalls::io {
    MatroskaWriter::MatroskaWriter(media::BaseSink* sink): BaseIO(sink), ThreadedWriter(sink) {
        if (const auto video_sink = dynamic_cast<media::VideoSink*>(sink)) {
            if (const auto description = video_sink->get_config()) {
                composer_ = std::make_unique<media::composer::VideoComposer>(description->width, description->height);
            }
        }
    }

    MatroskaWriter::~MatroskaWriter() {
        const std::lock_guard lock(state_mutex_);
        for (const auto& decoder : decoders_ | std::views::values) {
            decoder->Release();
        }
        if (encoder_) {
            encoder_->Release();
        }
    }

    void MatroskaWriter::send_frames(std::vector<wrtc::models::Frame> frames) {
        const std::lock_guard lock(state_mutex_);
        for (const auto& frame : frames) {
            if (!composing_) {
                if (!source_ssrc_) {
                    source_ssrc_ = frame.ssrc;
                }
                if (*source_ssrc_ != frame.ssrc || !media::containers::MatroskaMuxer::supports(frame.frame_data.codec)) {
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

    void MatroskaWriter::remove_source(const int64_t ssrc) {
        if (composer_) {
            composer_->remove(ssrc);
        }
    }

    void MatroskaWriter::forward(const wrtc::models::Frame& frame) {
        if (!muxer_) {
            codec_ = frame.frame_data.codec;
            muxer_ = std::make_unique<media::containers::MatroskaMuxer>(*codec_, frame.frame_data.width, frame.frame_data.height);
        }
        write_frame(frame.data.data(), frame.data.size(), frame.frame_data.absolute_capture_timestamp_ms, frame.frame_data.keyframe);
    }

    void MatroskaWriter::decode(const wrtc::models::Frame& frame) {
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

    bool MatroskaWriter::ensure_decoder(const int64_t ssrc, const wrtc::models::Codec codec) {
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

    int32_t MatroskaWriter::Decoded(webrtc::VideoFrame& decoded_image) {
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

    std::optional<std::pair<bytes::unique_binary, size_t>> MatroskaWriter::idle_data() {
        const std::lock_guard lock(state_mutex_);
        if (!composing_ || !composer_ || !ensure_encoder()) {
            return std::nullopt;
        }
        const auto description = dynamic_cast<media::VideoSink*>(sink_)->get_config();
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
        return std::nullopt;
    }

    bool MatroskaWriter::ensure_encoder() {
        if (encoder_) {
            return true;
        }
        const auto description = dynamic_cast<media::VideoSink*>(sink_)->get_config();
        if (!description) {
            return false;
        }
        const auto codec = codec_.value_or(wrtc::models::Codec::VP8);
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
        if (!muxer_) {
            codec_ = codec;
            muxer_ = std::make_unique<media::containers::MatroskaMuxer>(codec, settings.width, settings.height);
        }
        return true;
    }

    webrtc::EncodedImageCallback::Result MatroskaWriter::OnEncodedImage(const webrtc::EncodedImage& encoded_image, const webrtc::CodecSpecificInfo*) {
        write_frame(encoded_image.data(), encoded_image.size(), webrtc::TimeMillis(), encoded_image._frameType == webrtc::VideoFrameType::kVideoFrameKey);
        return Result(Result::OK);
    }

    void MatroskaWriter::OnFrameDropped(uint32_t, int, bool) {}

    void MatroskaWriter::write_frame(const uint8_t* data, const size_t size, const int64_t timestamp_ms, const bool keyframe) {
        auto output = muxer_->add(data, size, timestamp_ms, keyframe);
        if (output.empty()) {
            return;
        }
        auto buffer = bytes::make_unique_binary(output.size());
        std::memcpy(buffer.get(), output.data(), output.size());
        push(std::move(buffer), output.size());
    }

    std::chrono::nanoseconds MatroskaWriter::idle_interval() const {
        return sink_->frame_time();
    }

    webrtc::SdpVideoFormat MatroskaWriter::sdp_format(const wrtc::models::Codec codec) {
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

    webrtc::VideoCodecType MatroskaWriter::codec_type(const wrtc::models::Codec codec) {
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
} // ntgcalls::io
