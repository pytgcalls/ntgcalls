//
// Created by Lauren on 17/06/26.
//

#pragma once
#include <ntgcalls/e2e/permissions.hpp>
#include <ntgcalls/tl/e2e_api.hpp>

namespace ntgcalls::e2e {
    struct State {
        static constexpr tl::Hash256 kEmptyTrieHash = {
            0xdf, 0x3f, 0x61, 0x98, 0x04, 0xa9, 0x2f, 0xdb, 0x40, 0x57, 0x19, 0x2d, 0xc4, 0x3d, 0xd7, 0x48,
            0xea, 0x77, 0x8a, 0xdc, 0x52, 0xbc, 0x49, 0x8c, 0xe8, 0x05, 0x24, 0xc0, 0x14, 0xb8, 0x11, 0x19
        };

        chain::SharedKey shared_key;
        chain::GroupState group_state;
        tl::Hash256 kv_hash = kEmptyTrieHash;
        bool has_set_value = false;
        bool has_shared_key_change = false;
        bool has_group_state_change = false;

        static std::optional<chain::GroupParticipant> find_participant(const chain::GroupState& state, int64_t user_id);

        static std::optional<chain::GroupParticipant> find_participant(const chain::GroupState& state, const tl::PublicKeyBytes& public_key);

        static int32_t group_state_version(const chain::GroupState& state);

        static Permissions get_permissions(
            const chain::GroupState& state,
            const tl::PublicKeyBytes& public_key,
            int32_t limit_permissions
        );

        bool set_shared_key(const chain::SharedKey& new_key, const Permissions& permissions);

        [[nodiscard]] bool apply_change(const chain::Change& change, const tl::PublicKeyBytes& signer, int32_t limit_permissions);

        bool set_group_state(const chain::GroupState& new_state, const Permissions& permissions);

        static bool validate_group_state(const chain::GroupState& state);

        bool clear_shared_key(const Permissions& permissions);

        [[nodiscard]] bool validate_state(const chain::StateProof& proof) const;

        static bool validate_shared_key(const chain::SharedKey& shared_key, const chain::GroupState& state);

        static bool verify_block(const chain::Block& block, const tl::PublicKeyBytes& public_key);

        bool apply(const chain::Block& block, bool validate_signature, bool validate_state_hash, int32_t limit_permissions);
    };
} // ntgcalls::e2e
