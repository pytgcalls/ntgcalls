//
// Created by Lauren on 17/06/26.
//

#include <ntgcalls/e2e/chain/blockchain.hpp>
#include <ntgcalls/e2e/chain/client_blockchain.hpp>

namespace ntgcalls::e2e::chain {
    ClientBlockchain ClientBlockchain::create_empty() {
        ClientBlockchain res;
        res.blockchain_ = Blockchain::create_empty();
        return std::move(res);
    }

    std::optional<ClientBlockchain> ClientBlockchain::create_from_block(const bytes::const_span serialized_block) {
        tl::TlReader reader(serialized_block);
        const auto block = Block::fetch_boxed(reader);
        if (!reader.finish()) {
            return std::nullopt;
        }
        auto chain = Blockchain::create_from_block(block);
        if (!chain) {
            return std::nullopt;
        }
        ClientBlockchain result;
        result.blockchain_ = std::move(*chain);
        return result;
    }

    const GroupState & ClientBlockchain::group_state() const {
        return blockchain_.current_group_state();
    }

    const Blockchain & ClientBlockchain::inner() const {
        return blockchain_;
    }

    int32_t ClientBlockchain::height() const {
        return blockchain_.height();
    }

    tl::Hash256 ClientBlockchain::previous_block_hash() const {
        return blockchain_.previous_hash();
    }

    tl::Hash256 ClientBlockchain::last_block_hash() const {
        return blockchain_.hash();
    }

    std::optional<std::vector<Change>> ClientBlockchain::try_apply_block(const bytes::const_span serialized_block) {
        tl::TlReader reader(serialized_block);
        auto block = Block::fetch_boxed(reader);
        if (!reader.finish()) {
            return std::nullopt;
        }
        if (!blockchain_.try_apply_block(block, true, false)) {
            return std::nullopt;
        }
        return std::move(block.changes);
    }

    const SharedKey & ClientBlockchain::group_shared_key() const {
        return blockchain_.current_shared_key();
    }

    std::optional<bytes::binary> ClientBlockchain::build_block(const std::vector<Change> &changes, const openssl::Key25519 &key) const {
        const auto block = blockchain_.build_block(changes, key);
        if (!block) {
            return std::nullopt;
        }
        tl::TlWriter writer;
        block->store_boxed(writer);
        return writer.result();
    }
} // ntgcalls::e2e::chain
