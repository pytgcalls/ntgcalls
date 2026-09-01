//
// Created by Lauren on 17/06/26.
//

#include <memory>
#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <wrtc/utils/encryption.hpp>

namespace ntgcalls::e2e::chain {
    Blockchain Blockchain::create_empty() {
        Blockchain result;
        result.last_block_.height = -1;
        return result;
    }

    const GroupState& Blockchain::current_group_state() const {
        return state_.group_state;
    }

    std::optional<Block> Blockchain::build_block(std::vector<Change> changes, const openssl::Key25519& key) const {
        const auto public_key = key.public_key_bytes();
        auto working_state = state_;
        if (last_block_.height == std::numeric_limits<int32_t>::max()) {
            return std::nullopt;
        }
        const int32_t new_height = last_block_.height + 1;
        if (new_height == 0) {
            working_state.group_state = GroupState{
                {},
                Permissions::AllPermissions
            };
        }
        for (const auto& change : changes) {
            if (!working_state.apply_change(change, public_key, Permissions::AllPermissions)) {
                return std::nullopt;
            }
        }

        StateProof proof;
        proof.kv_hash = working_state.kv_hash;
        proof.group_state = working_state.group_state;
        proof.shared_key = working_state.shared_key;
        working_state.has_set_value = false;
        working_state.has_group_state_change = false;
        working_state.has_shared_key_change = false;
        for (const auto& [value] : changes) {
            std::visit([&]<typename T>(const T&) {
                if constexpr (std::is_same_v<T, ChangeSetGroupState>) {
                    proof.group_state = std::nullopt;
                    proof.shared_key = std::nullopt;
                    working_state.has_group_state_change = true;
                } else if constexpr (std::is_same_v<T, ChangeSetSharedKey>) {
                    proof.shared_key = std::nullopt;
                    working_state.has_shared_key_change = true;
                } else if constexpr (std::is_same_v<T, ChangeSetValue>) {
                    working_state.has_set_value = true;
                }
            },
                       value);
        }
        if (!working_state.validate_state(proof)) {
            return std::nullopt;
        }

        Block block;
        block.height = new_height;
        block.prev_block_hash = last_block_hash_;
        block.changes = std::move(changes);
        block.signature_public_key = public_key;
        block.state_proof = std::move(proof);
        sign_block(block, key);
        return block;
    }

    std::optional<Blockchain> Blockchain::create_from_block(Block block) {
        if (block.height < 0) {
            return std::nullopt;
        }
        Blockchain result;
        result.last_block_hash_ = calc_hash(block);

        State state_return;
        if (block.height == 0) {
            state_return.group_state = GroupState{{}, Permissions::AllPermissions};
        }
        for (const auto& [valueChange] : block.changes) {
            std::visit([&]<typename T>(const T& value) {
                if constexpr (std::is_same_v<T, ChangeSetGroupState>) {
                    state_return.group_state = value.group_state;
                    state_return.shared_key = {};
                    state_return.has_group_state_change = true;
                } else if constexpr (std::is_same_v<T, ChangeSetSharedKey>) {
                    state_return.shared_key = value.shared_key;
                    state_return.has_shared_key_change = true;
                } else if constexpr (std::is_same_v<T, ChangeSetValue>) {
                    state_return.has_set_value = true;
                }
            },
                       valueChange);
        }
        if (block.state_proof.group_state) {
            state_return.group_state = *block.state_proof.group_state;
        }
        if (block.state_proof.shared_key) {
            state_return.shared_key = *block.state_proof.shared_key;
        }
        state_return.kv_hash = block.state_proof.kv_hash;
        if (!state_return.validate_state(block.state_proof)) {
            return std::nullopt;
        }
        result.state_ = std::move(state_return);
        result.last_block_ = std::move(block);
        return result;
    }

    tl::Hash256 Blockchain::previous_hash() const {
        return last_block_.prev_block_hash;
    }

    const SharedKey& Blockchain::current_shared_key() const {
        return state_.shared_key;
    }

    void Blockchain::sign_block(Block& block, const openssl::Key25519& key) {
        block.signature = key.sign(bytes::view(data_to_sign(block)));
    }

    bytes::binary Blockchain::data_to_sign(const Block& block) {
        Block copy = block;
        copy.signature = {};
        tl::TlWriter w;
        copy.store_boxed(w);
        return w.result();
    }

    bool Blockchain::try_apply_block(const Block& block, const bool validate_signature, const bool validate_state_hash) {
        if (block.height != height() + 1 || height() == std::numeric_limits<int32_t>::max()) {
            return false;
        }
        if (block.prev_block_hash != last_block_hash_) {
            return false;
        }
        auto working_state = state_;
        if (!working_state.apply(block, validate_signature, validate_state_hash, Permissions::AllPermissions)) {
            return false;
        }
        state_ = std::move(working_state);
        last_block_hash_ = calc_hash(block);
        last_block_ = block;
        return true;
    }

    tl::Hash256 Blockchain::calc_hash(const Block& block) {
        tl::Hash256 result{};
        if (block.height == -1) {
            return result;
        }
        tl::TlWriter writer;
        block.store_boxed(writer);
        const auto digest = openssl::Sha256::digest(bytes::view(writer.result()));
        std::copy_n(digest.data(), result.size(), result.begin());
        return result;
    }

    int32_t Blockchain::height() const {
        return last_block_.height;
    }

    tl::Hash256 Blockchain::hash() const {
        return last_block_hash_;
    }
} // ntgcalls::e2e::chain
