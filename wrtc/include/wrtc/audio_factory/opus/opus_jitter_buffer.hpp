//
// Created by Lauren on 22/11/26.
//

#pragma once
#include <deque>
#include <map>
#include <optional>
#include <vector>
#include <wrtc/utils/encoded/encoded_frame.hpp>

struct OpusDecoder;
struct OpusEncoder;

namespace wrtc::audio_factory {
    class OpusJitterBuffer {
        static constexpr int kPacketSamples = 960;
        static constexpr int64_t kPacketMs = 20;
        static constexpr int64_t kMinDelayMs = 20;
        static constexpr int64_t kMarginMs = 20;
        static constexpr int64_t kResetMs = 2000;
        static constexpr size_t kDelayWindow = 500;
        static constexpr size_t kDelayPercentile = 99;
        static constexpr int64_t kMaxConcealedPackets = 5;
        static constexpr size_t kHistoryPackets = 2;

        std::map<int64_t, bytes::binary> pending_;
        std::deque<bytes::binary> history_;
        std::deque<int64_t> delays_;
        std::optional<int64_t> next_ms_;
        OpusDecoder* decoder_ = nullptr;
        OpusEncoder* encoder_ = nullptr;
        int channels_ = 0;

        void reset();

        int64_t base_delay() const;

        int64_t target_delay(int64_t max_delay_ms) const;

        bool ensure_codecs(int channels);

        std::vector<int16_t> decode(const bytes::binary* packet, int samples, bool fec) const;

        std::vector<utils::encoded::EncodedFrame> conceal(int64_t packets, const bytes::binary& next, int64_t start_ms);

        void release(std::vector<utils::encoded::EncodedFrame>& output, std::map<int64_t, bytes::binary>::iterator it);

    public:
        static constexpr int64_t kDefaultMaxDelayMs = 1000;

        ~OpusJitterBuffer();

        void push(utils::encoded::EncodedFrame frame);

        std::vector<utils::encoded::EncodedFrame> pop(int64_t now_ms, int64_t max_delay_ms);
    };
} // wrtc::audio_factory
