//
// Created by Lauren on 22/11/26.
//

#include <algorithm>
#include <array>
#include <third_party/opus/src/include/opus.h>
#include <wrtc/audio_factory/opus/opus_jitter_buffer.hpp>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace wrtc::audio_factory {
    OpusJitterBuffer::~OpusJitterBuffer() {
        if (decoder_) {
            opus_decoder_destroy(decoder_);
        }
        if (encoder_) {
            opus_encoder_destroy(encoder_);
        }
    }

    void OpusJitterBuffer::push(utils::encoded::EncodedFrame frame) {
        const int64_t capture = frame.frame_data.absolute_capture_timestamp_ms;
        if (next_ms_ && capture < *next_ms_ - kResetMs) {
            reset();
        }
        delays_.push_back(frame.arrival_ms - capture);
        if (delays_.size() > kDelayWindow) {
            delays_.pop_front();
        }
        if (next_ms_ && capture < *next_ms_ - kPacketMs / 2) {
            return;
        }
        pending_.try_emplace(capture, std::move(frame.payload));
    }

    std::vector<utils::encoded::EncodedFrame> OpusJitterBuffer::pop(const int64_t now_ms, const int64_t max_delay_ms) {
        std::vector<utils::encoded::EncodedFrame> output;
        const int64_t delay = base_delay() + target_delay(max_delay_ms);
        while (!pending_.empty()) {
            const auto it = pending_.begin();
            if (!next_ms_) {
                next_ms_ = it->first;
            }
            if (it->first <= *next_ms_ + kPacketMs / 2) {
                release(output, it);
                continue;
            }
            if (now_ms < *next_ms_ + delay) {
                break;
            }
            if (const int64_t packets = (it->first - *next_ms_ + kPacketMs / 2) / kPacketMs; packets <= kMaxConcealedPackets) {
                auto concealed = conceal(packets, it->second, *next_ms_);
                std::ranges::move(concealed, std::back_inserter(output));
            }
            next_ms_ = it->first;
        }
        return output;
    }

    void OpusJitterBuffer::release(std::vector<utils::encoded::EncodedFrame>& output, const std::map<int64_t, bytes::binary>::iterator it) {
        if (it->first >= *next_ms_ - kPacketMs / 2) {
            const int samples = OpusPacket::samples(it->second.data(), it->second.size());
            output.push_back({it->second, {it->first, webrtc::kVideoRotation_0, 0, 0, models::Codec::Opus, true}});
            next_ms_ = it->first + samples * 1000 / OpusPacket::kSampleRate;
            history_.push_back(std::move(it->second));
            if (history_.size() > kHistoryPackets) {
                history_.pop_front();
            }
        }
        pending_.erase(it);
    }

    void OpusJitterBuffer::reset() {
        pending_.clear();
        history_.clear();
        delays_.clear();
        next_ms_.reset();
    }

    int64_t OpusJitterBuffer::base_delay() const {
        return delays_.empty() ? 0 : std::ranges::min(delays_);
    }

    int64_t OpusJitterBuffer::target_delay(const int64_t max_delay_ms) const {
        const int64_t floor = std::min(kMinDelayMs, max_delay_ms);
        if (delays_.empty()) {
            return floor;
        }
        std::vector sorted(delays_.begin(), delays_.end());
        const auto nth = sorted.begin() + static_cast<std::ptrdiff_t>((sorted.size() - 1) * kDelayPercentile / 100);
        std::ranges::nth_element(sorted, nth);
        return std::clamp(*nth - base_delay() + kMarginMs, floor, max_delay_ms);
    }

    bool OpusJitterBuffer::ensure_codecs(const int channels) {
        if (decoder_ && encoder_ && channels_ == channels) {
            return true;
        }
        if (decoder_) {
            opus_decoder_destroy(decoder_);
            decoder_ = nullptr;
        }
        if (encoder_) {
            opus_encoder_destroy(encoder_);
            encoder_ = nullptr;
        }
        int error = 0;
        decoder_ = opus_decoder_create(OpusPacket::kSampleRate, channels, &error);
        if (error != OPUS_OK) {
            decoder_ = nullptr;
            return false;
        }
        encoder_ = opus_encoder_create(OpusPacket::kSampleRate, channels, OPUS_APPLICATION_AUDIO, &error);
        if (error != OPUS_OK) {
            encoder_ = nullptr;
            return false;
        }
        channels_ = channels;
        return true;
    }

    std::vector<int16_t> OpusJitterBuffer::decode(const bytes::binary* packet, const int samples, const bool fec) const {
        std::vector<int16_t> pcm(static_cast<size_t>(std::max(samples, 0)) * channels_, 0);
        if (samples <= 0) {
            return pcm;
        }
        if (opus_decode(decoder_, packet ? packet->data() : nullptr, packet ? static_cast<opus_int32>(packet->size()) : 0, pcm.data(), samples, fec ? 1 : 0) < 0) {
            std::ranges::fill(pcm, 0);
        }
        return pcm;
    }

    std::vector<utils::encoded::EncodedFrame> OpusJitterBuffer::conceal(const int64_t packets, const bytes::binary& next, const int64_t start_ms) {
        std::vector<utils::encoded::EncodedFrame> output;
        if (!ensure_codecs(static_cast<int>(OpusPacket::channels(next.data(), next.size())))) {
            return output;
        }
        opus_decoder_ctl(decoder_, OPUS_RESET_STATE);
        std::vector<int16_t> pcm;
        for (const auto& packet : history_) {
            const auto decoded = decode(&packet, OpusPacket::samples(packet.data(), packet.size()), false);
            pcm.insert(pcm.end(), decoded.begin(), decoded.end());
        }
        const size_t primed = pcm.size() / channels_;
        const bool fec = opus_packet_has_lbrr(next.data(), static_cast<opus_int32>(next.size())) == 1;
        for (int64_t i = 0; i < packets; i++) {
            const bool recover = fec && i == packets - 1;
            const auto decoded = decode(recover ? &next : nullptr, kPacketSamples, recover);
            pcm.insert(pcm.end(), decoded.begin(), decoded.end());
        }
        const int next_samples = OpusPacket::samples(next.data(), next.size());
        const auto decoded = decode(&next, next_samples, false);
        pcm.insert(pcm.end(), decoded.begin(), decoded.end());

        opus_encoder_ctl(encoder_, OPUS_RESET_STATE);
        opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(std::clamp(static_cast<opus_int32>(next.size() * 8 * OpusPacket::kSampleRate / std::max(next_samples, 1)), 6000, 510000)));
        opus_int32 lookahead = 0;
        opus_encoder_ctl(encoder_, OPUS_GET_LOOKAHEAD(&lookahead));
        const size_t start = primed % kPacketSamples + static_cast<size_t>(lookahead);
        const size_t warmup = primed / kPacketSamples;
        for (size_t i = 0; i < warmup + static_cast<size_t>(packets); i++) {
            const size_t offset = (start + i * kPacketSamples) * channels_;
            if (offset + static_cast<size_t>(kPacketSamples) * channels_ > pcm.size()) {
                break;
            }
            std::array<uint8_t, 4000> packet{};
            const int size = opus_encode(encoder_, pcm.data() + offset, kPacketSamples, packet.data(), static_cast<opus_int32>(packet.size()));
            if (i < warmup || size <= 0) {
                continue;
            }
            const int64_t timestamp = start_ms + static_cast<int64_t>(i - warmup) * kPacketMs;
            output.push_back({{packet.data(), packet.data() + size}, {timestamp, webrtc::kVideoRotation_0, 0, 0, models::Codec::Opus, true}});
        }
        return output;
    }
} // wrtc::audio_factory
