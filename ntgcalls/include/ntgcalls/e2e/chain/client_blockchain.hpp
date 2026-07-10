//
// Created by Lauren on 17/06/26.
//

#pragma once
#include <optional>
#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/key25519.hpp>

namespace ntgcalls::e2e::chain {
    class ClientBlockchain {
        Blockchain blockchain_;

    public:
        static ClientBlockchain create_empty();

        static std::optional<ClientBlockchain> create_from_block(bytes::const_span serialized_block);

        [[nodiscard]] const GroupState& group_state() const;

        [[nodiscard]] const Blockchain& inner() const;

        [[nodiscard]] int32_t height() const;

        [[nodiscard]] tl::Hash256 previous_block_hash() const;

        [[nodiscard]] tl::Hash256 last_block_hash() const;

        std::optional<std::vector<Change>> try_apply_block(bytes::const_span serialized_block);

        [[nodiscard]] const SharedKey& group_shared_key() const;

        [[nodiscard]] std::optional<bytes::binary> build_block(const std::vector<Change>& changes, const openssl::Key25519& key) const;
    };
} // ntgcalls::e2e::chain
