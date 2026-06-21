//
// Created by Laky64 on 07/06/26.
//

#pragma once
#include <array>
#include <string>
#include <cstdint>
#include <wrtc/utils/binary.hpp>

namespace telegram {
    using PublicKeyBytes = std::array<uint8_t, 32>;
    using Hash256 = std::array<uint8_t, 32>;

    class TlWriter {
        bytes::binary buffer;

    public:
        void storeInt32(int32_t value);

        void storeUInt32(uint32_t value);

        void storeInt64(int64_t value);

        void storeInt256(const std::array<uint8_t, 32>& value);

        void storeInt512(const std::array<uint8_t, 64>& value);

        void storeBytes(bytes::const_span value);

        void storeString(const std::string& value);

        void storeRaw(bytes::const_span value);

        void storeVectorSize(uint32_t size);

        [[nodiscard]] bytes::binary result();
    };

    class TlReader {
        const uint8_t* ptr;
        const uint8_t* end;
        bool failed = false;

    public:
        explicit TlReader(bytes::const_span data);

        int32_t fetchInt32();

        uint32_t fetchUInt32();

        int64_t fetchInt64();

        std::array<uint8_t, 32> fetchInt256();

        std::array<uint8_t, 64> fetchInt512();

        bytes::binary fetchBytes();

        std::string fetchString();

        bytes::binary fetchRaw(size_t size);

        uint32_t fetchVectorSize();

        [[nodiscard]] bool ok() const;

        void setError();

        [[nodiscard]] bool finish() const;
    };
} // ntgcalls::e2e
