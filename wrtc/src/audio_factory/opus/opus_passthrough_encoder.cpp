//
// Created by Lauren on 19/11/26.
//

#include <rtc_base/logging.h>
#include <third_party/opus/src/include/opus.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>
#include <wrtc/audio_factory/opus/opus_passthrough_encoder.hpp>

namespace wrtc::audio_factory {
    OpusPassthroughEncoder::OpusPassthroughEncoder(std::shared_ptr<utils::encoded::EncodedQueue> queue, std::unique_ptr<AudioEncoder> encoder, const int payload_type, const size_t channels, const std::optional<uint32_t> max_bitrate):
    queue_(std::move(queue)), encoder_(std::move(encoder)), payload_type_(payload_type), channels_(channels), max_bitrate_(max_bitrate) {}

    OpusPassthroughEncoder::~OpusPassthroughEncoder() {
        if (decoder_) {
            opus_decoder_destroy(decoder_);
        }
    }

    bool OpusPassthroughEncoder::compatible(const bytes::binary& packet) {
        if (OpusPacket::channels(packet.data()) != channels_) {
            return false;
        }
        if (!max_bitrate_) {
            return true;
        }
        window_bits_ += static_cast<int64_t>(packet.size()) * 8;
        window_ms_ += static_cast<int64_t>(OpusPacket::frames(packet.data(), packet.size())) * 10;
        if (++window_packets_ < kBitrateWindow) {
            return true;
        }
        const auto bitrate = window_bits_ * 1000 / window_ms_;
        window_bits_ = 0;
        window_ms_ = 0;
        window_packets_ = 0;
        return bitrate <= static_cast<int64_t>(*max_bitrate_) * 11 / 10;
    }

    int OpusPassthroughEncoder::SampleRateHz() const {
        return kSampleRate;
    }

    size_t OpusPassthroughEncoder::NumChannels() const {
        return channels_;
    }

    size_t OpusPassthroughEncoder::Num10MsFramesInNextPacket() const {
        return transcoding_ ? encoder_->Num10MsFramesInNextPacket() : pending_frames_;
    }

    size_t OpusPassthroughEncoder::Max10MsFramesInAPacket() const {
        return std::max(kMaxFrames, encoder_->Max10MsFramesInAPacket());
    }

    int OpusPassthroughEncoder::GetTargetBitrate() const {
        return encoder_->GetTargetBitrate();
    }

    void OpusPassthroughEncoder::Reset() {
        pending_ = std::nullopt;
        elapsed_frames_ = 0;
        decoded_.clear();
        encoder_->Reset();
    }

    bool OpusPassthroughEncoder::SetFec(const bool enable) {
        return encoder_->SetFec(enable);
    }

    bool OpusPassthroughEncoder::SetDtx(const bool enable) {
        return encoder_->SetDtx(enable);
    }

    void OpusPassthroughEncoder::OnReceivedUplinkPacketLossFraction(const float uplink_packet_loss_fraction) {
        encoder_->OnReceivedUplinkPacketLossFraction(uplink_packet_loss_fraction);
    }

    void OpusPassthroughEncoder::OnReceivedTargetAudioBitrate(const int target_bps) {
        encoder_->OnReceivedTargetAudioBitrate(target_bps);
    }

    void OpusPassthroughEncoder::OnReceivedUplinkAllocation(const webrtc::BitrateAllocationUpdate update) {
        encoder_->OnReceivedUplinkAllocation(update);
    }

    void OpusPassthroughEncoder::OnReceivedRtt(const int rtt_ms) {
        encoder_->OnReceivedRtt(rtt_ms);
    }

    void OpusPassthroughEncoder::OnReceivedOverhead(const size_t overhead_bytes_per_packet) {
        encoder_->OnReceivedOverhead(overhead_bytes_per_packet);
    }

    std::optional<std::pair<webrtc::TimeDelta, webrtc::TimeDelta>> OpusPassthroughEncoder::GetFrameLengthRange() const {
        return encoder_->GetFrameLengthRange();
    }

    webrtc::AudioEncoder::EncodedInfo OpusPassthroughEncoder::EncodeImpl(const uint32_t rtp_timestamp, std::span<const int16_t>, webrtc::Buffer* encoded) {
        if (transcoding_) {
            return transcode(rtp_timestamp, encoded);
        }
        return forward(rtp_timestamp, encoded);
    }

    webrtc::AudioEncoder::EncodedInfo OpusPassthroughEncoder::forward(const uint32_t rtp_timestamp, webrtc::Buffer* encoded) {
        if (!pending_) {
            pending_ = queue_->pop();
            if (!pending_) {
                return {};
            }
            if (!compatible(pending_->payload)) {
                RTC_LOG(LS_INFO) << "Opus packets don't match the requested channels or bitrate, re-encoding them";
                transcoding_ = true;
                queue_->push_front(std::move(*pending_));
                pending_ = std::nullopt;
                return transcode(rtp_timestamp, encoded);
            }
            pending_frames_ = std::min(OpusPacket::frames(pending_->payload.data(), pending_->payload.size()), kMaxFrames);
            if (pending_frames_ == 0) {
                pending_ = std::nullopt;
                return {};
            }
            first_timestamp_ = rtp_timestamp;
            elapsed_frames_ = 0;
        }
        if (++elapsed_frames_ < pending_frames_) {
            return {};
        }
        encoded->AppendData(pending_->payload.data(), pending_->payload.size());
        EncodedInfo info;
        info.encoded_bytes = pending_->payload.size();
        info.encoded_timestamp = first_timestamp_;
        info.payload_type = payload_type_;
        info.encoder_type = CodecType::kOpus;
        pending_ = std::nullopt;
        return info;
    }

    webrtc::AudioEncoder::EncodedInfo OpusPassthroughEncoder::transcode(const uint32_t rtp_timestamp, webrtc::Buffer* encoded) {
        const auto frame_size = static_cast<size_t>(kFrameSamples) * channels_;
        while (decoded_.size() < frame_size) {
            const auto packet = queue_->pop();
            if (!packet) {
                break;
            }
            if (!decoder_) {
                int error = 0;
                decoder_ = opus_decoder_create(kSampleRate, static_cast<int>(channels_), &error);
                if (error != OPUS_OK) {
                    decoder_ = nullptr;
                    return {};
                }
            }
            std::vector<int16_t> pcm(static_cast<size_t>(kMaxPacketSamples) * channels_);
            if (const int samples = opus_decode(decoder_, packet->payload.data(), static_cast<opus_int32>(packet->payload.size()), pcm.data(), kMaxPacketSamples, 0); samples > 0) {
                decoded_.insert(decoded_.end(), pcm.begin(), pcm.begin() + static_cast<std::ptrdiff_t>(samples * channels_));
            }
        }
        if (decoded_.size() < frame_size) {
            decoded_.resize(frame_size, 0);
        }
        const std::span<const int16_t> frame(decoded_.data(), frame_size);
        auto info = encoder_->Encode(rtp_timestamp, frame, encoded);
        decoded_.erase(decoded_.begin(), decoded_.begin() + static_cast<std::ptrdiff_t>(frame_size));
        return info;
    }
} // wrtc::audio_factory
