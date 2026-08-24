//
// Created by Lauren on 08/03/24.
//

#pragma once
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::signaling::crypto {
    struct EncryptionKey {
        static constexpr int kSize = 256;

        std::shared_ptr<const bytes::array<kSize>> value;
        bool is_outgoing = false;

        EncryptionKey(
            std::shared_ptr<const bytes::array<kSize>> const& value,
            const bool is_outgoing
        ): value(value), is_outgoing(is_outgoing) {}
    };
    using RawKey = bytes::array<EncryptionKey::kSize>;

    class AuthKey {
    public:
        static bytes::binary create_auth_key(bytes::const_span first_bytes, bytes::const_span random, bytes::const_span prime_bytes);

        static void fill_data(RawKey& auth_key, bytes::const_span computed_auth_key);

        static uint64_t fingerprint(bytes::const_span auth_key);
    };
} // ntgcalls::signaling::crypto
