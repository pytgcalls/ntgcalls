//
// Created by Laky-64 on 17/06/26.
//

#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <ntgcalls/e2e/chain/client_blockchain.hpp>

namespace telegram::e2e::chain {
    ClientBlockchain ClientBlockchain::createEmpty() {
        ClientBlockchain res;
        res.blockchain = Blockchain::createEmpty();
        return std::move(res);
    }

    std::optional<ClientBlockchain> ClientBlockchain::createFromBlock(const bytes::const_span serializedBlock) {
        TlReader reader(serializedBlock);
        const auto block = Block::fetchBoxed(reader);
        if (!reader.finish()) {
            return std::nullopt;
        }
        auto chain = Blockchain::createFromBlock(block);
        if (!chain) {
            return std::nullopt;
        }
        ClientBlockchain result;
        result.blockchain = std::move(*chain);
        return result;
    }

    const GroupState & ClientBlockchain::groupState() const {
        return blockchain.currentGroupState();
    }

    const Blockchain & ClientBlockchain::inner() const {
        return blockchain;
    }

    int32_t ClientBlockchain::height() const {
        return blockchain.height();
    }

    Hash256 ClientBlockchain::previousBlockHash() const {
        return blockchain.previousHash();
    }

    Hash256 ClientBlockchain::lastBlockHash() const {
        return blockchain.hash();
    }

    std::optional<std::vector<Change>> ClientBlockchain::tryApplyBlock(const bytes::const_span serializedBlock) {
        TlReader reader(serializedBlock);
        auto block = Block::fetchBoxed(reader);
        if (!reader.finish()) {
            return std::nullopt;
        }
        if (!blockchain.tryApplyBlock(block, true, false)) {
            return std::nullopt;
        }
        return std::move(block.changes);
    }

    const SharedKey & ClientBlockchain::groupSharedKey() const {
        return blockchain.currentSharedKey();
    }

    std::optional<bytes::binary> ClientBlockchain::buildBlock(const std::vector<Change> &changes, const openssl::Key25519 &key) const {
        const auto block = blockchain.buildBlock(changes, key);
        if (!block) {
            return std::nullopt;
        }
        TlWriter writer;
        block->storeBoxed(writer);
        return writer.result();
    }
} // telegram::e2e::chain
