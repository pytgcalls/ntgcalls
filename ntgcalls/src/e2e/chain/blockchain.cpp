//
// Created by Laky-64 on 17/06/26.
//

#include <memory>
#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <wrtc/utils/encryption.hpp>

namespace telegram::e2e::chain {
    Blockchain Blockchain::createEmpty() {
        Blockchain result;
        result.lastBlock.height = -1;
        return result;
    }

    const GroupState & Blockchain::currentGroupState() const {
        return state.groupState;
    }

    std::optional<Block> Blockchain::buildBlock(std::vector<Change> changes, const openssl::Key25519 &key) const {
        const auto publicKey = key.publicKeyBytes();
        auto workingState = state;
        if (lastBlock.height == std::numeric_limits<int32_t>::max()) {
            return std::nullopt;
        }
        const int32_t newHeight = lastBlock.height + 1;
        if (newHeight == 0) {
            workingState.groupState = GroupState{
                {},
                Permissions::AllPermissions
            };
        }
        for (const auto& change : changes) {
            if (!workingState.applyChange(change, publicKey, Permissions::AllPermissions)) {
                return std::nullopt;
            }
        }

        StateProof proof;
        proof.kv_hash = workingState.kvHash;
        proof.group_state = workingState.groupState;
        proof.shared_key = workingState.sharedKey;
        workingState.hasSetValue = false;
        workingState.hasGroupStateChange = false;
        workingState.hasSharedKeyChange = false;
        for (const auto&[value] : changes) {
            std::visit([&]<typename T>(const T&) {
                if constexpr (std::is_same_v<T, ChangeSetGroupState>) {
                    proof.group_state = std::nullopt;
                    proof.shared_key = std::nullopt;
                    workingState.hasGroupStateChange = true;
                } else if constexpr (std::is_same_v<T, ChangeSetSharedKey>) {
                    proof.shared_key = std::nullopt;
                    workingState.hasSharedKeyChange = true;
                } else if constexpr (std::is_same_v<T, ChangeSetValue>) {
                    workingState.hasSetValue = true;
                }
            }, value);
        }
        if (!workingState.validateState(proof)) {
            return std::nullopt;
        }

        Block block;
        block.height = newHeight;
        block.prev_block_hash = lastBlockHash;
        block.changes = std::move(changes);
        block.signature_public_key = publicKey;
        block.state_proof = std::move(proof);
        signBlock(block, key);
        return block;
    }

    std::optional<Blockchain> Blockchain::createFromBlock(Block block) {
        if (block.height < 0) {
            return std::nullopt;
        }
        Blockchain result;
        result.lastBlockHash = calcHash(block);

        State stateReturn;
        if (block.height == 0) {
            stateReturn.groupState = GroupState{{}, Permissions::AllPermissions};
        }
        for (const auto&[valueChange] : block.changes) {
            std::visit([&]<typename T>(const T& value) {
                if constexpr (std::is_same_v<T, ChangeSetGroupState>) {
                    stateReturn.groupState = value.group_state;
                    stateReturn.sharedKey = {};
                    stateReturn.hasGroupStateChange = true;
                } else if constexpr (std::is_same_v<T, ChangeSetSharedKey>) {
                    stateReturn.sharedKey = value.shared_key;
                    stateReturn.hasSharedKeyChange = true;
                } else if constexpr (std::is_same_v<T, ChangeSetValue>) {
                    stateReturn.hasSetValue = true;
                }
            }, valueChange);
        }
        if (block.state_proof.group_state) {
            stateReturn.groupState = *block.state_proof.group_state;
        }
        if (block.state_proof.shared_key) {
            stateReturn.sharedKey = *block.state_proof.shared_key;
        }
        stateReturn.kvHash = block.state_proof.kv_hash;
        if (!stateReturn.validateState(block.state_proof)) {
            return std::nullopt;
        }
        result.state = std::move(stateReturn);
        result.lastBlock = std::move(block);
        return result;
    }

    Hash256 Blockchain::previousHash() const {
        return lastBlock.prev_block_hash;
    }

    const SharedKey & Blockchain::currentSharedKey() const {
        return state.sharedKey;
    }

    void Blockchain::signBlock(Block& block, const openssl::Key25519& key) {
        block.signature = key.sign(bytes::view(dataToSign(block)));
    }

    bytes::binary Blockchain::dataToSign(const Block &block) {
        Block copy = block;
        copy.signature = {};
        TlWriter w;
        copy.storeBoxed(w);
        return w.result();
    }

    bool Blockchain::tryApplyBlock(const Block& block, const bool validateSignature, const bool validateStateHash) {
        if (block.height != height() + 1 || height() == std::numeric_limits<int32_t>::max()) {
            return false;
        }
        if (block.prev_block_hash != lastBlockHash) {
            return false;
        }
        auto workingState = state;
        if (!workingState.apply(block, validateSignature, validateStateHash, Permissions::AllPermissions)) {
            return false;
        }
        state = std::move(workingState);
        lastBlockHash = calcHash(block);
        lastBlock = block;
        return true;
    }

    Hash256 Blockchain::calcHash(const Block &block) {
        Hash256 result{};
        if (block.height == -1) {
            return result;
        }
        TlWriter writer;
        block.storeBoxed(writer);
        const auto digest = openssl::Sha256::Digest(bytes::view(writer.result()));
        std::copy_n(reinterpret_cast<const uint8_t*>(digest.data()), result.size(), result.begin());
        return result;
    }

    int32_t Blockchain::height() const {
        return lastBlock.height;
    }

    Hash256 Blockchain::hash() const {
        return lastBlockHash;
    }
} // telegram::e2e::chain