//
// Created by Laky-64 on 17/06/26.
//

#pragma once
#include <optional>
#include <ntgcalls/e2e/state.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <wrtc/utils/key25519.hpp>

namespace telegram::e2e::chain {
    class Blockchain {
        State state;
        Block lastBlock;
        Hash256 lastBlockHash{};

        static void signBlock(Block &block, const openssl::Key25519 &key);

        static Hash256 calcHash(const Block& block);

    public:
        static Blockchain createEmpty();

        static bytes::binary dataToSign(const Block& block);

        [[nodiscard]] const GroupState& currentGroupState() const;

        [[nodiscard]] std::optional<Block> buildBlock(std::vector<Change> changes, const openssl::Key25519& key) const;

        static std::optional<Blockchain> createFromBlock(Block block);

        bool tryApplyBlock(const Block &block, bool validateSignature, bool validateStateHash);

        [[nodiscard]] int32_t height() const;

        [[nodiscard]] Hash256 hash() const;

        [[nodiscard]] Hash256 previousHash() const;

        [[nodiscard]] const SharedKey& currentSharedKey() const;
    };
} // telegram::e2e::chain
