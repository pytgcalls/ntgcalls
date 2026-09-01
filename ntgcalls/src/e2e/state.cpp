//
// Created by Lauren on 17/06/26.
//

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <ntgcalls/e2e/state.hpp>
#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <wrtc/utils/key25519.hpp>

namespace ntgcalls::e2e {
    std::optional<chain::GroupParticipant> State::find_participant(const chain::GroupState& state, const int64_t user_id) {
        for (const auto& participant : state.participants) {
            if (participant.user_id == user_id) {
                return participant;
            }
        }
        return std::nullopt;
    }

    std::optional<chain::GroupParticipant> State::find_participant(
        const chain::GroupState& state,
        const tl::PublicKeyBytes& public_key
    ) {
        for (const auto& participant : state.participants) {
            if (participant.public_key == public_key) {
                return participant;
            }
        }
        return std::nullopt;
    }

    int32_t State::group_state_version(const chain::GroupState& state) {
        if (state.participants.empty()) {
            return 0;
        }
        int32_t result = state.participants.front().version;
        for (const auto& participant : state.participants) {
            result = std::min(result, participant.version);
        }
        return std::clamp(result, 0, 255);
    }

    Permissions State::get_permissions(
        const chain::GroupState& state,
        const tl::PublicKeyBytes& public_key,
        int32_t limit_permissions
    ) {
        limit_permissions &= Permissions::AllPermissions;
        if (const auto participant = find_participant(state, public_key)) {
            return Permissions{participant->flags() & limit_permissions | Permissions::IsParticipant};
        }
        return Permissions{state.external_permissions & limit_permissions};
    }

    bool State::set_shared_key(const chain::SharedKey& new_key, const Permissions& permissions) {
        if (shared_key != chain::SharedKey{}) {
            return false;
        }
        if (!permissions.may_change_shared_key()) {
            return false;
        }
        if (!validate_shared_key(new_key, group_state)) {
            return false;
        }
        shared_key = new_key;
        return true;
    }

    bool State::apply_change(const chain::Change& change, const tl::PublicKeyBytes& signer, const int32_t limit_permissions) {
        return std::visit([&]<typename T>(const T& value) {
            if constexpr (std::is_same_v<T, chain::ChangeNoop>) {
                return true;
            } else if constexpr (std::is_same_v<T, chain::ChangeSetValue>) {
                has_set_value = true;
                return true;
            } else if constexpr (std::is_same_v<T, chain::ChangeSetGroupState>) {
                has_group_state_change = true;
                if (!set_group_state(value.group_state, get_permissions(group_state, signer, limit_permissions))) {
                    return false;
                }
                return clear_shared_key(get_permissions(group_state, signer, limit_permissions));
            } else if constexpr (std::is_same_v<T, chain::ChangeSetSharedKey>) {
                has_shared_key_change = true;
                return set_shared_key(value.shared_key, get_permissions(group_state, signer, limit_permissions));
            } else {
                return false;
            }
        },
                          change.value);
    }

    bool State::set_group_state(const chain::GroupState& new_state, const Permissions& permissions) {
        if (!validate_group_state(new_state)) {
            return false;
        }
        std::map<std::pair<int64_t, tl::PublicKeyBytes>, int32_t> old_participants;
        std::map<std::pair<int64_t, tl::PublicKeyBytes>, int32_t> new_participants;
        for (const auto& p : group_state.participants) {
            old_participants[{p.user_id, p.public_key}] = p.flags();
        }
        for (const auto& p : new_state.participants) {
            new_participants[{p.user_id, p.public_key}] = p.flags();
        }
        if ((~group_state.external_permissions & new_state.external_permissions) != 0) {
            return false;
        }
        int32_t needed_flags = 0;
        for (const auto& key : old_participants | std::views::keys) {
            if (!new_participants.contains(key) && !permissions.may_remove_users()) {
                return false;
            }
        }
        for (const auto& [key, flags] : new_participants) {
            if (const auto it = old_participants.find(key); it == old_participants.end()) {
                if (!permissions.may_add_users()) {
                    return false;
                }
                needed_flags |= flags;
            } else if (flags != it->second) {
                if (!permissions.may_add_users() || !permissions.may_remove_users()) {
                    return false;
                }
                needed_flags |= flags & ~it->second;
            }
        }
        if ((needed_flags & ~(permissions.flags & Permissions::AllPermissions)) != 0) {
            return false;
        }
        group_state = new_state;
        return true;
    }

    bool State::validate_group_state(const chain::GroupState& state) {
        std::set<int64_t> user_ids;
        std::set<tl::PublicKeyBytes> keys;
        for (const auto& participant : state.participants) {
            if ((participant.flags() & ~Permissions::AllPermissions) != 0) {
                return false;
            }
            user_ids.insert(participant.user_id);
            keys.insert(participant.public_key);
        }
        if ((state.external_permissions & ~Permissions::AllPermissions) != 0) {
            return false;
        }
        return user_ids.size() == state.participants.size() && keys.size() == state.participants.size();
    }

    bool State::clear_shared_key(const Permissions& permissions) {
        if (!permissions.may_change_shared_key()) {
            return false;
        }
        shared_key = {};
        return true;
    }

    bool State::validate_state(const chain::StateProof& proof) const {
        if (proof.kv_hash != kv_hash) {
            return false;
        }
        if (!has_group_state_change && !has_set_value) {
            return false;
        }
        if (has_group_state_change && proof.group_state) {
            return false;
        }
        if (!has_group_state_change && !proof.group_state) {
            return false;
        }
        if (!has_group_state_change && *proof.group_state != group_state) {
            return false;
        }
        const bool shared_key_omitted = has_group_state_change || has_shared_key_change;
        if (shared_key_omitted && proof.shared_key) {
            return false;
        }
        if (!shared_key_omitted && !proof.shared_key) {
            return false;
        }
        if (!shared_key_omitted && *proof.shared_key != shared_key) {
            return false;
        }
        return validate_group_state(group_state) && validate_shared_key(shared_key, group_state);
    }

    bool State::validate_shared_key(const chain::SharedKey& shared_key, const chain::GroupState& state) {
        if (shared_key == chain::SharedKey{}) {
            return true;
        }
        if (shared_key.dest_user_id.size() != shared_key.dest_header.size()) {
            return false;
        }
        if (shared_key.dest_user_id.size() != state.participants.size()) {
            return false;
        }
        std::set participants(shared_key.dest_user_id.begin(), shared_key.dest_user_id.end());
        if (participants.size() != shared_key.dest_user_id.size()) {
            return false;
        }
        return std::ranges::all_of(state.participants, [&participants](const chain::GroupParticipant& p) {
            return participants.contains(p.user_id);
        });
    }

    bool State::verify_block(const chain::Block& block, const tl::PublicKeyBytes& public_key) {
        return openssl::Key25519::verify(
            bytes::view(public_key),
            bytes::view(chain::Blockchain::data_to_sign(block)),
            bytes::view(block.signature)
        );
    }

    // ReSharper disable once CppDFAConstantFunctionResult
    bool State::apply(
        const chain::Block& block,
        const bool validate_signature,
        const bool validate_state_hash,
        const int32_t limit_permissions
    ) {
        if (block.height == 0) {
            group_state = chain::GroupState{{}, Permissions::AllPermissions};
        }
        std::optional<tl::PublicKeyBytes> signer = block.signature_public_key;
        if (!signer && group_state != chain::GroupState{}) {
            signer = group_state.participants[0].public_key;
        }
        if (!signer) {
            return false;
        }
        if (validate_signature && !verify_block(block, *signer)) {
            return false;
        }
        has_set_value = false;
        has_shared_key_change = false;
        has_group_state_change = false;
        for (const auto& change : block.changes) {
            if (!apply_change(change, *signer, limit_permissions)) {
                return false;
            }
        }
        if (!validate_state_hash) {
            kv_hash = block.state_proof.kv_hash;
        }
        return validate_state(block.state_proof);
    }
} // ntgcalls::e2e
