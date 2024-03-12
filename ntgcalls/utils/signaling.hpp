//
// Created by Laky64 on 09/03/2024.
//
#pragma once
#include <cstdint>
#include <rtc_base/byte_buffer.h>

#include "wrtc/utils/binary.hpp"

namespace ntgcalls {

    class Signaling {
        uint64_t counter = 0;
        bool isOutGoing = false;
        bytes::binary key;
        std::vector<uint32_t> _largestIncomingCounters;

        static constexpr auto kSingleMessagePacketSeqBit = static_cast<uint32_t>(1) << 31;
        static constexpr auto kMessageRequiresAckSeqBit = static_cast<uint32_t>(1) << 30;
        static constexpr auto kKeepIncomingCountersCount = 64;
        static constexpr auto kMaxIncomingPacketSize = 128 * 1024;

        [[nodiscard]] bytes::binary encryptPrepared(const bytes::binary& buffer) const;

        static uint32_t ReadSeq(const void* bytes);

        static uint32_t CounterFromSeq(uint32_t seq);

        static bool ConstTimeIsDifferent(const void *a, const void *b, size_t size);

        bool registerIncomingCounter(uint32_t incomingCounter);

    public:
        Signaling(bool isOutGoing, bytes::binary key);

        ~Signaling();

        bytes::binary encrypt(const bytes::binary& buffer);

        bytes::binary decrypt(const bytes::binary& buffer);
    };

} // ntgcalls

