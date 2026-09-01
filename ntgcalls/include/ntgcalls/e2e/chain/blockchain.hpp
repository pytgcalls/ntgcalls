//
// Created by Lauren on 17/06/26.
//

#pragma once
#include <optional>
#include <ntgcalls/e2e/state.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <wrtc/utils/key25519.hpp>

namespace ntgcalls::e2e::chain {
    class Blockchain {
        State state_;
        Block last_block_;
        tl::Hash256 last_block_hash_{};

        static void sign_block(Block& block, const openssl::Key25519& key);

        static tl::Hash256 calc_hash(const Block& block);

    public:
        static Blockchain create_empty();

        static bytes::binary data_to_sign(const Block& block);

        [[nodiscard]] const GroupState& current_group_state() const;

        [[nodiscard]] std::optional<Block> build_block(std::vector<Change> changes, const openssl::Key25519& key) const;

        static std::optional<Blockchain> create_from_block(Block block);

        bool try_apply_block(const Block& block, bool validate_signature, bool validate_state_hash);

        [[nodiscard]] int32_t height() const;

        [[nodiscard]] tl::Hash256 hash() const;

        [[nodiscard]] tl::Hash256 previous_hash() const;

        [[nodiscard]] const SharedKey& current_shared_key() const;
    };
} // ntgcalls::e2e::chain
