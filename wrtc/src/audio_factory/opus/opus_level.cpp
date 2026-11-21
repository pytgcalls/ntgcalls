//
// Created by Lauren on 20/11/26.
//

#include <algorithm>
#include <cmath>
#include <vector>
#include <wrtc/audio_factory/opus/opus_level.hpp>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace wrtc::audio_factory {
    OpusLevel::OpusLevel(const size_t channels): channels_(static_cast<int>(channels)) {
        int error = 0;
        mode_ = opus_custom_mode_create(OpusPacket::kSampleRate, 960, &error);
        if (error != OPUS_OK) {
            mode_ = nullptr;
        }
    }

    OpusLevel::~OpusLevel() {
        if (decoder_) {
            opus_decoder_destroy(decoder_);
        }
    }

    int OpusLevel::end_band(const int bandwidth) {
        switch (bandwidth) {
        case OPUS_BANDWIDTH_NARROWBAND:
            return 13;
        case OPUS_BANDWIDTH_MEDIUMBAND:
        case OPUS_BANDWIDTH_WIDEBAND:
            return 17;
        case OPUS_BANDWIDTH_SUPERWIDEBAND:
            return 19;
        default:
            return kBands;
        }
    }

    int OpusLevel::frame_lm(const int samples) {
        switch (samples) {
        case 120:
            return 0;
        case 240:
            return 1;
        case 480:
            return 2;
        default:
            return 3;
        }
    }

    bool OpusLevel::parse_transient(ec_dec* dec, const int lm, const int total) {
        if (ec_tell(dec) + 16 <= total && ec_dec_bit_logp(dec, 1)) {
            const int octave = static_cast<int>(ec_dec_uint(dec, 6));
            ec_dec_bits(dec, 4 + octave);
            ec_dec_bits(dec, 3);
            if (ec_tell(dec) + 2 <= total) {
                ec_dec_icdf(dec, kTapsetIcdf, 2);
            }
        }
        return lm > 0 && ec_tell(dec) + 3 <= total && ec_dec_bit_logp(dec, 3);
    }

    void OpusLevel::parse_coarse(ec_dec* dec, const int lm, const int channels, const int end) {
        const int total = static_cast<int>(dec->storage * 8);
        const bool intra = ec_tell(dec) + 3 <= total && ec_dec_bit_logp(dec, 3);
        const auto model = kProbModel[lm][intra];
        const float coef = intra ? 0 : kPredictionCoef[lm];
        const float beta = intra ? kBetaIntra : kBetaCoef[lm];
        std::array<float, 2> prev = {0, 0};
        for (int i = 0; i < end; i++) {
            for (int c = 0; c < channels; c++) {
                int qi;
                if (const int budget = total - ec_tell(dec); budget >= 15) {
                    const int pi = 2 * std::min(i, 20);
                    qi = ec_laplace_decode(dec, model[pi] << 7, model[pi + 1] << 6);
                } else if (budget >= 2) {
                    qi = ec_dec_icdf(dec, kSmallEnergyIcdf, 2);
                    qi = (qi >> 1) ^ -(qi & 1);
                } else if (budget >= 1) {
                    qi = -ec_dec_bit_logp(dec, 1);
                } else {
                    qi = -1;
                }
                const auto q = static_cast<float>(qi);
                auto& energy = energy_[c][i];
                energy = coef * std::max(-9.f, energy) + prev[c] + q;
                prev[c] += q - beta * q;
            }
        }
    }

    void OpusLevel::parse_fine(ec_dec* dec, const int lm, const int channels, const int end, const int size, const bool transient) {
        const int total = size * 8;
        auto budget = static_cast<opus_uint32>(total);
        auto tell = static_cast<opus_uint32>(ec_tell(dec));
        int logp = transient ? 2 : 4;
        const bool select_reserved = lm > 0 && tell + logp + 1 <= budget;
        budget -= select_reserved;
        int current = 0;
        int changed = 0;
        for (int i = 0; i < end; i++) {
            if (tell + logp <= budget) {
                current ^= ec_dec_bit_logp(dec, logp);
                tell = static_cast<opus_uint32>(ec_tell(dec));
                changed |= current;
            }
            logp = transient ? 4 : 5;
        }
        if (select_reserved && kTfSelect[lm][4 * transient + changed] != kTfSelect[lm][4 * transient + 2 + changed]) {
            ec_dec_bit_logp(dec, 1);
        }
        if (ec_tell(dec) + 4 <= total) {
            ec_dec_icdf(dec, kSpreadIcdf, 5);
        }
        std::array<int, kBands> caps{}, offsets{}, fine{}, pulses{}, priority{};
        for (int i = 0; i < kBands; i++) {
            const int width = (kBandEdges[i + 1] - kBandEdges[i]) << lm;
            caps[i] = (kCaps[kBands * (2 * lm + channels - 1) + i] + 64) * channels * width >> 2;
        }
        int dynalloc_logp = 6;
        opus_int32 total_frac = total << BITRES;
        auto tell_frac = static_cast<opus_int32>(ec_tell_frac(dec));
        for (int i = 0; i < end; i++) {
            const int width = channels * (kBandEdges[i + 1] - kBandEdges[i]) << lm;
            const int quanta = std::min(width << BITRES, std::max(6 << BITRES, width));
            int loop_logp = dynalloc_logp;
            int boost = 0;
            while (tell_frac + (loop_logp << BITRES) < total_frac && boost < caps[i]) {
                const int flag = ec_dec_bit_logp(dec, loop_logp);
                tell_frac = static_cast<opus_int32>(ec_tell_frac(dec));
                if (!flag) {
                    break;
                }
                boost += quanta;
                total_frac -= quanta;
                loop_logp = 1;
            }
            offsets[i] = boost;
            if (boost > 0) {
                dynalloc_logp = std::max(2, dynalloc_logp - 1);
            }
        }
        const int trim = tell_frac + (6 << BITRES) <= total_frac ? ec_dec_icdf(dec, kTrimIcdf, 7) : 5;
        opus_int32 bits = (static_cast<opus_int32>(total) << BITRES) - static_cast<opus_int32>(ec_tell_frac(dec)) - 1;
        if (transient && lm >= 2 && bits >= (lm + 2) << BITRES) {
            bits -= 1 << BITRES;
        }
        int intensity = 0;
        int dual_stereo = 0;
        opus_int32 balance = 0;
        clt_compute_allocation(mode_, 0, end, offsets.data(), caps.data(), trim, &intensity, &dual_stereo, bits, &balance, pulses.data(), fine.data(), priority.data(), channels, lm, dec, 0, 0, 0);
        for (int i = 0; i < end; i++) {
            if (fine[i] <= 0) {
                continue;
            }
            for (int c = 0; c < channels; c++) {
                const auto q2 = static_cast<float>(ec_dec_bits(dec, fine[i]));
                energy_[c][i] += (q2 + .5f) * static_cast<float>(1 << (14 - fine[i])) / 16384.f - .5f;
            }
        }
    }

    double OpusLevel::celt_power(const unsigned char* data, const int size, const int lm, const int channels, const int end) {
        ec_dec dec;
        ec_dec_init(&dec, const_cast<unsigned char*>(data), size);
        if (channels == 1) {
            for (int i = 0; i < kBands; i++) {
                energy_[0][i] = std::max(energy_[0][i], energy_[1][i]);
            }
        }
        const int total = size * 8;
        if (const int tell = ec_tell(&dec); tell >= total || (tell == 1 && ec_dec_bit_logp(&dec, 15))) {
            return 0;
        }
        const bool transient = parse_transient(&dec, lm, total);
        parse_coarse(&dec, lm, channels, end);
        if (mode_) {
            parse_fine(&dec, lm, channels, end, size, transient);
        }
        double power = 0;
        for (int c = 0; c < channels; c++) {
            for (int i = 0; i < end; i++) {
                const double amplitude = std::exp2(energy_[c][i] + kBandMeans[i]);
                power += amplitude * amplitude * kDeemphasisGain[i];
            }
        }
        return power * kPowerScale / channels;
    }

    double OpusLevel::decoded_power(const uint8_t* data, const size_t size) {
        if (!decoder_) {
            int error = 0;
            decoder_ = opus_decoder_create(OpusPacket::kSampleRate, channels_, &error);
            if (error != OPUS_OK) {
                decoder_ = nullptr;
                return 0;
            }
        }
        std::vector<int16_t> pcm(static_cast<size_t>(kMaxPacketSamples) * channels_);
        const int samples = opus_decode(decoder_, data, static_cast<opus_int32>(size), pcm.data(), kMaxPacketSamples, 0);
        if (samples <= 0) {
            return 0;
        }
        double power = 0;
        for (int i = 0; i < samples * channels_; i++) {
            power += static_cast<double>(pcm[i]) * pcm[i];
        }
        return power / (samples * channels_);
    }

    int16_t OpusLevel::amplitude(const uint8_t* data, const size_t size) {
        double power;
        if ((data[0] >> 3) < 16) {
            power = decoded_power(data, size);
        } else {
            const unsigned char* frames[kMaxFrames];
            opus_int16 sizes[kMaxFrames];
            const int count = opus_packet_parse(data, static_cast<opus_int32>(size), nullptr, frames, sizes, nullptr);
            if (count <= 0) {
                return 0;
            }
            const int lm = frame_lm(opus_packet_get_samples_per_frame(data, OpusPacket::kSampleRate));
            const int channels = opus_packet_get_nb_channels(data);
            const int end = end_band(opus_packet_get_bandwidth(data));
            power = 0;
            for (int i = 0; i < count; i++) {
                power += celt_power(frames[i], sizes[i], lm, channels, end);
            }
            power /= count;
        }
        return static_cast<int16_t>(std::min(std::sqrt(power), 32767.0));
    }
} // wrtc::audio_factory
