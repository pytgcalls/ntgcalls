//
// Created by Laky-64 on 17/06/26.
//

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <ntgcalls/e2e/state.hpp>
#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <wrtc/utils/key25519.hpp>

namespace telegram::e2e {
    std::optional<chain::GroupParticipant> State::findParticipant(const chain::GroupState &state, const int64_t userId) {
        for (const auto& participant : state.participants) {
            if (participant.user_id == userId) {
                return participant;
            }
        }
        return std::nullopt;
    }

    std::optional<chain::GroupParticipant> State::findParticipant(
        const chain::GroupState &state,
        const PublicKeyBytes& publicKey
    ) {
        for (const auto& participant : state.participants) {
            if (participant.public_key == publicKey) {
                return participant;
            }
        }
        return std::nullopt;
    }

    int32_t State::groupStateVersion(const chain::GroupState &state) {
        if (state.participants.empty()) {
            return 0;
        }
        int32_t result = state.participants.front().version;
        for (const auto& participant : state.participants) {
            result = std::min(result, participant.version);
        }
        return std::clamp(result, 0, 255);
    }

    Permissions State::getPermissions(
        const chain::GroupState &state,
        const PublicKeyBytes& publicKey,
        int32_t limitPermissions
    ) {
        limitPermissions &= Permissions::AllPermissions;
        if (const auto participant = findParticipant(state, publicKey)) {
            return Permissions{participant->flags & limitPermissions | Permissions::IsParticipant};
        }
        return Permissions{state.external_permissions & limitPermissions};
    }

    bool State::setSharedKey(const chain::SharedKey &newKey, const Permissions &permissions) {
        if (sharedKey != chain::SharedKey{}) {
            return false;
        }
        if (!permissions.mayChangeSharedKey()) {
            return false;
        }
        if (!validateSharedKey(newKey, groupState)) {
            return false;
        }
        sharedKey = newKey;
        return true;
    }

    bool State::applyChange(const chain::Change &change, const PublicKeyBytes& signer, const int32_t limitPermissions) {
        return std::visit([&]<typename T>(const T& value) {
            if constexpr (std::is_same_v<T, chain::ChangeNoop>) {
                return true;
            } else if constexpr (std::is_same_v<T, chain::ChangeSetValue>) {
                hasSetValue = true;
                return true;
            } else if constexpr (std::is_same_v<T, chain::ChangeSetGroupState>) {
                hasGroupStateChange = true;
                if (!setGroupState(value.group_state, getPermissions(groupState, signer, limitPermissions))) {
                    return false;
                }
                return clearSharedKey(getPermissions(groupState, signer, limitPermissions));
            } else if constexpr (std::is_same_v<T, chain::ChangeSetSharedKey>) {
                hasSharedKeyChange = true;
                return setSharedKey(value.shared_key, getPermissions(groupState, signer, limitPermissions));
            } else {
                return false;
            }
        }, change.value);
    }

    bool State::setGroupState(const chain::GroupState &newState, const Permissions &permissions) {
        if (!validateGroupState(newState)) {
            return false;
        }
        std::map<std::pair<int64_t, PublicKeyBytes>, int32_t> oldParticipants;
        std::map<std::pair<int64_t, PublicKeyBytes>, int32_t> newParticipants;
        for (const auto& p : groupState.participants) {
            oldParticipants[{p.user_id, p.public_key}] = p.flags;
        }
        for (const auto& p : newState.participants) {
            newParticipants[{p.user_id, p.public_key}] = p.flags;
        }
        if ((~groupState.external_permissions & newState.external_permissions) != 0) {
            return false;
        }
        int32_t neededFlags = 0;
        for (const auto &key: oldParticipants | std::views::keys) {
            if (!newParticipants.contains(key) && !permissions.mayRemoveUsers()) {
                return false;
            }
        }
        for (const auto& [key, flags] : newParticipants) {
            if (const auto it = oldParticipants.find(key); it == oldParticipants.end()) {
                if (!permissions.mayAddUsers()) {
                    return false;
                }
                neededFlags |= flags;
            } else if (flags != it->second) {
                if (!permissions.mayAddUsers() || !permissions.mayRemoveUsers()) {
                    return false;
                }
                neededFlags |= flags & ~it->second;
            }
        }
        if ((neededFlags & ~(permissions.flags & Permissions::AllPermissions)) != 0) {
            return false;
        }
        groupState = newState;
        return true;
    }

    bool State::validateGroupState(const chain::GroupState &state) {
        std::set<int64_t> userIds;
        std::set<PublicKeyBytes> keys;
        for (const auto& participant : state.participants) {
            if ((participant.flags & ~Permissions::AllPermissions) != 0) {
                return false;
            }
            userIds.insert(participant.user_id);
            keys.insert(participant.public_key);
        }
        if ((state.external_permissions & ~Permissions::AllPermissions) != 0) {
            return false;
        }
        return userIds.size() == state.participants.size() && keys.size() == state.participants.size();
    }

    bool State::clearSharedKey(const Permissions &permissions) {
        if (!permissions.mayChangeSharedKey()) {
            return false;
        }
        sharedKey = {};
        return true;
    }

    bool State::validateState(const chain::StateProof &proof) const {
        if (proof.kv_hash != kvHash) {
            return false;
        }
        if (!hasGroupStateChange && !hasSetValue) {
            return false;
        }
        if (hasGroupStateChange && proof.group_state) {
            return false;
        }
        if (!hasGroupStateChange && !proof.group_state) {
            return false;
        }
        if (!hasGroupStateChange && *proof.group_state != groupState) {
            return false;
        }
        const bool sharedKeyOmitted = hasGroupStateChange || hasSharedKeyChange;
        if (sharedKeyOmitted && proof.shared_key) {
            return false;
        }
        if (!sharedKeyOmitted && !proof.shared_key) {
            return false;
        }
        if (!sharedKeyOmitted && *proof.shared_key != sharedKey) {
            return false;
        }
        return validateGroupState(groupState) && validateSharedKey(sharedKey, groupState);
    }

    bool State::validateSharedKey(const chain::SharedKey &sharedKey, const chain::GroupState &state) {
        if (sharedKey == chain::SharedKey{}) {
            return true;
        }
        if (sharedKey.dest_user_id.size() != sharedKey.dest_header.size()) {
            return false;
        }
        if (sharedKey.dest_user_id.size() != state.participants.size()) {
            return false;
        }
        std::set participants(sharedKey.dest_user_id.begin(), sharedKey.dest_user_id.end());
        if (participants.size() != sharedKey.dest_user_id.size()) {
            return false;
        }
        return std::ranges::all_of(state.participants, [&participants](const chain::GroupParticipant& p) {
            return participants.contains(p.user_id);
        });
    }

    bool State::verifyBlock(const chain::Block &block, const PublicKeyBytes &publicKey) {
        return openssl::Key25519::Verify(
            bytes::view(publicKey),
            bytes::view(chain::Blockchain::dataToSign(block)),
            bytes::view(block.signature)
        );
    }

    // ReSharper disable once CppDFAConstantFunctionResult
    bool State::apply(
        const chain::Block &block,
        const bool validateSignature,
        const bool validateStateHash,
        const int32_t limitPermissions
    ) {
        if (block.height == 0) {
            groupState = chain::GroupState{{}, Permissions::AllPermissions};
        }
        std::optional<PublicKeyBytes> signer = block.signature_public_key;
        if (!signer && groupState != chain::GroupState{}) {
            signer = groupState.participants[0].public_key;
        }
        if (!signer) {
            return false;
        }
        if (validateSignature && !verifyBlock(block, *signer)) {
            return false;
        }
        hasSetValue = false;
        hasSharedKeyChange = false;
        hasGroupStateChange = false;
        for (const auto& change : block.changes) {
            if (!applyChange(change, *signer, limitPermissions)) {
                return false;
            }
        }
        if (!validateStateHash) {
            kvHash = block.state_proof.kv_hash;
        }
        return validateState(block.state_proof);
    }
} // telegram