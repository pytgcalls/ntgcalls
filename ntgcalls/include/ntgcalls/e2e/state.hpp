//
// Created by Laky-64 on 17/06/26.
//

#pragma once
#include <ntgcalls/e2e/permissions.hpp>
#include <ntgcalls/tl/e2e_api.hpp>

namespace telegram::e2e {
    struct State {
        static constexpr Hash256 kEmptyTrieHash = {
            0xdf, 0x3f, 0x61, 0x98, 0x04, 0xa9, 0x2f, 0xdb, 0x40, 0x57, 0x19, 0x2d, 0xc4, 0x3d, 0xd7, 0x48,
            0xea, 0x77, 0x8a, 0xdc, 0x52, 0xbc, 0x49, 0x8c, 0xe8, 0x05, 0x24, 0xc0, 0x14, 0xb8, 0x11, 0x19
        };

        chain::SharedKey sharedKey;
        chain::GroupState groupState;
        Hash256 kvHash = kEmptyTrieHash;
        bool hasSetValue = false;
        bool hasSharedKeyChange = false;
        bool hasGroupStateChange = false;

        static std::optional<chain::GroupParticipant> findParticipant(const chain::GroupState& state, int64_t userId);

        static std::optional<chain::GroupParticipant> findParticipant(const chain::GroupState& state, const PublicKeyBytes& publicKey);

        static int32_t groupStateVersion(const chain::GroupState& state);

        static Permissions getPermissions(
            const chain::GroupState& state,
            const PublicKeyBytes& publicKey,
            int32_t limitPermissions
        );

        bool setSharedKey(const chain::SharedKey& newKey, const Permissions& permissions);

        [[nodiscard]] bool applyChange(const chain::Change& change, const PublicKeyBytes& signer, int32_t limitPermissions);

        bool setGroupState(const chain::GroupState& newState, const Permissions& permissions);

        static bool validateGroupState(const chain::GroupState& state);

        bool clearSharedKey(const Permissions& permissions);

        [[nodiscard]] bool validateState(const chain::StateProof& proof) const;

        static bool validateSharedKey(const chain::SharedKey& sharedKey, const chain::GroupState& state);

        static bool verifyBlock(const chain::Block& block, const PublicKeyBytes& publicKey);

        bool apply(const chain::Block& block, bool validateSignature, bool validateStateHash, int32_t limitPermissions);
    };
} // telegram
