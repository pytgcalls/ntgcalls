//
// Created by Lauren on 19/11/26.
//

#pragma once
#include <api/audio_codecs/audio_encoder.h>
#include <wrtc/utils/encoded/encoded_queue.hpp>

struct OpusDecoder;

namespace wrtc::audio_factory {
    class OpusPassthroughEncoder final: public webrtc::AudioEncoder {
        static constexpr int kSampleRate = 48000;
        static constexpr int kFrameSamples = kSampleRate / 100;
        static constexpr size_t kMaxFrames = 12;
        static constexpr int kMaxPacketSamples = 5760;
        static constexpr int64_t kBitrateWindowMs = 10000;

        std::shared_ptr<utils::encoded::EncodedQueue> queue_;
        std::unique_ptr<AudioEncoder> encoder_;
        int payload_type_;
        size_t channels_;
        std::optional<uint32_t> max_bitrate_;
        std::optional<utils::encoded::EncodedFrame> pending_;
        size_t pending_frames_ = 2;
        size_t elapsed_frames_ = 0;
        uint32_t first_timestamp_ = 0;
        bool transcoding_ = false;
        int64_t window_bits_ = 0;
        int64_t window_ms_ = 0;
        OpusDecoder* decoder_ = nullptr;
        std::vector<int16_t> decoded_;

        bool compatible(const bytes::binary& packet);

        EncodedInfo forward(uint32_t rtp_timestamp, webrtc::Buffer* encoded);

        EncodedInfo transcode(uint32_t rtp_timestamp, webrtc::Buffer* encoded);

    public:
        OpusPassthroughEncoder(std::shared_ptr<utils::encoded::EncodedQueue> queue, std::unique_ptr<AudioEncoder> encoder, int payload_type, size_t channels, std::optional<uint32_t> max_bitrate);

        ~OpusPassthroughEncoder() override;

        int SampleRateHz() const override;

        size_t NumChannels() const override;

        size_t Num10MsFramesInNextPacket() const override;

        size_t Max10MsFramesInAPacket() const override;

        int GetTargetBitrate() const override;

        void Reset() override;

        bool SetFec(bool enable) override;

        bool SetDtx(bool enable) override;

        void OnReceivedUplinkPacketLossFraction(float uplink_packet_loss_fraction) override;

        void OnReceivedTargetAudioBitrate(int target_bps) override;

        void OnReceivedUplinkAllocation(webrtc::BitrateAllocationUpdate update) override;

        void OnReceivedRtt(int rtt_ms) override;

        void OnReceivedOverhead(size_t overhead_bytes_per_packet) override;

        std::optional<std::pair<webrtc::TimeDelta, webrtc::TimeDelta>> GetFrameLengthRange() const override;

    protected:
        EncodedInfo EncodeImpl(uint32_t rtp_timestamp, std::span<const int16_t> audio, webrtc::Buffer* encoded) override;
    };
} // wrtc::audio_factory
