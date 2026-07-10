//
// Created by Lauren on 07/06/26.
//

#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::tl {
    using PublicKeyBytes = bytes::array<32>;
    using Hash256 = bytes::array<32>;

    class TlWriter {
        bytes::binary buffer_;

    public:
        void store_int32(int32_t value);

        void store_u_int32(uint32_t value);

        void store_int64(int64_t value);

        void store_int256(const bytes::array<32>& value);

        void store_int512(const bytes::array<64>& value);

        void store_bytes(bytes::const_span value);

        void store_string(const std::string& value);

        void store_raw(bytes::const_span value);

        void store_vector_size(uint32_t size);

        [[nodiscard]] bytes::binary result();
    };

    class TlReader {
        const uint8_t* ptr_;
        const uint8_t* end_;
        bool failed_ = false;

    public:
        explicit TlReader(bytes::const_span data);

        int32_t fetch_int32();

        uint32_t fetch_uint32();

        int64_t fetch_int64();

        std::array<uint8_t, 32> fetch_int256();

        std::array<uint8_t, 64> fetch_int512();

        bytes::binary fetch_bytes();

        std::string fetch_string();

        bytes::binary fetch_raw(size_t size);

        uint32_t fetch_vector_size();

        [[nodiscard]] bool ok() const;

        void set_error();

        [[nodiscard]] bool finish() const;
    };
} // ntgcalls::e2e
