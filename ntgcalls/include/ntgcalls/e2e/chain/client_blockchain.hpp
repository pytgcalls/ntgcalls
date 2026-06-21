//
// Created by Laky-64 on 17/06/26.
//

#pragma once
#include <optional>
#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <ntgcalls/tl/e2e_api.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/key25519.hpp>

namespace telegram::e2e::chain {
    class ClientBlockchain {
        Blockchain blockchain;

    public:
        static ClientBlockchain createEmpty();

        static std::optional<ClientBlockchain> createFromBlock(bytes::const_span serializedBlock);

        [[nodiscard]] const GroupState& groupState() const;

        [[nodiscard]] const Blockchain& inner() const;

        [[nodiscard]] int32_t height() const;

        [[nodiscard]] Hash256 previousBlockHash() const;

        [[nodiscard]] Hash256 lastBlockHash() const;

        std::optional<std::vector<Change>> tryApplyBlock(bytes::const_span serializedBlock);

        [[nodiscard]] const SharedKey& groupSharedKey() const;

        [[nodiscard]] std::optional<bytes::binary> buildBlock(const std::vector<Change>& changes, const openssl::Key25519& key) const;
    };
} // telegram::e2e::chain
