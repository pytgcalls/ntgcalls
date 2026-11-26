//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <optional>
#include <wrtc/utils/binary.hpp>

struct OpusEncoder;

namespace ntgcalls::media::codecs {

    class OpusPacketEncoder {
        OpusEncoder* encoder_ = nullptr;
        uint8_t channels_;
        bytes::binary silence_;

    public:
        static constexpr int kFrameSamples = 960;

        OpusPacketEncoder(uint8_t channels, std::optional<uint32_t> bitrate);

        ~OpusPacketEncoder();

        OpusPacketEncoder(const OpusPacketEncoder&) = delete;

        OpusPacketEncoder& operator=(const OpusPacketEncoder&) = delete;

        bytes::binary encode(const int16_t* pcm) const;

        [[nodiscard]] const bytes::binary& silence() const;
    };

} // ntgcalls::media::codecs
