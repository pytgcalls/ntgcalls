//
// Created by Laky-64 on 17/06/26.
//

#include <api/units/time_delta.h>
#include <ntgcalls/e2e/session.hpp>
#include <ntgcalls/e2e/chain/client_blockchain.hpp>
#include <ntgcalls/e2e/chain/message_encryption.hpp>
#include <ntgcalls/utils/emoji_fingerprint.hpp>
#include <wrtc/utils/random.hpp>

namespace telegram::e2e {
    Session::Session(wrtc::SafeThread& updateThread, int64_t userID) : updateThread(updateThread) {
        privateKey = openssl::Key25519::Generate();
        sessionEncryption = std::make_unique<SessionEncryption>(userID, privateKey);
        sessionVerification = std::make_unique<SessionVerification>(userID, privateKey);
    }

    Session::~Session() {
        std::lock_guard lock(mutex);
        sessionEncryption = nullptr;
        sessionVerification = nullptr;
    }

    std::array<uint8_t, 32> Session::randomSecret() {
        std::array<uint8_t, 32> secret{};
        bytes::RandomFill(bytes::span(reinterpret_cast<std::byte*>(secret.data()), secret.size()));
        return secret;
    }

    std::vector<chain::Change> Session::makeChangesForNewState(const chain::GroupState &groupState) {
        const auto ephemeralKey = openssl::Key25519::Generate();
        const auto groupSharedKey = randomSecret();
        const auto oneTimeSecret = randomSecret();
        const auto encryptedGroupSharedKey = chain::MessageEncryption::encryptData(
            bytes::view(groupSharedKey),
            bytes::view(oneTimeSecret)
        );
        chain::SharedKey sharedKey;
        sharedKey.ek = ephemeralKey.publicKeyBytes();
        sharedKey.encrypted_shared_key.assign(
            reinterpret_cast<const char*>(encryptedGroupSharedKey.data()),
            encryptedGroupSharedKey.size()
        );
        for (const auto& participant : groupState.participants) {
            const auto sharedSecret = ephemeralKey.computeSharedSecret(bytes::view(participant.public_key));
            const auto header = chain::MessageEncryption::encryptHeader(
                bytes::view(oneTimeSecret),
                bytes::view(encryptedGroupSharedKey),
                bytes::view(sharedSecret)
            );
            if (!header) {
                return {};
            }
            sharedKey.dest_user_id.push_back(participant.user_id);
            sharedKey.dest_header.push_back(*header);
        }
        return {
            chain::Change{
                chain::ChangeSetGroupState{groupState}
            },
            chain::Change{
                chain::ChangeSetSharedKey{std::move(sharedKey)}
            }
        };
    }

    bool Session::isGoodMagic(const int32_t magic) {
        return magic == chain::Block::ID || magic == chain::GroupBroadcastNonceCommit::ID || magic == chain::GroupBroadcastNonceReveal::ID;
    }

    int32_t Session::readMagic(const bytes::const_span block) {
        const auto data = reinterpret_cast<const uint8_t*>(block.data());
        return static_cast<int32_t>(
            static_cast<uint32_t>(data[0]) |
            static_cast<uint32_t>(data[1]) << 8 |
            static_cast<uint32_t>(data[2]) << 16 |
            static_cast<uint32_t>(data[3]) << 24);
    }

    void Session::writeMagic(bytes::binary &block, const int32_t magic) {
        const auto raw = static_cast<uint32_t>(magic);
        block[0] = static_cast<uint8_t>(raw & 0xff);
        block[1] = static_cast<uint8_t>(raw >> 8 & 0xff);
        block[2] = static_cast<uint8_t>(raw >> 16 & 0xff);
        block[3] = static_cast<uint8_t>(raw >> 24 & 0xff);
    }

    std::optional<bytes::binary> Session::fromServerToLocal(const bytes::const_span block) {
        if (block.size() < 4) {
            return std::nullopt;
        }
        const auto serverMagic = readMagic(block);
        if (isGoodMagic(serverMagic)) {
            return std::nullopt;
        }
        bytes::binary result(reinterpret_cast<const uint8_t*>(block.data()), reinterpret_cast<const uint8_t*>(block.data()) + block.size());
        writeMagic(result, serverMagic - 1);
        return result;
    }

    std::optional<bytes::binary> Session::createZeroBlock(
        const openssl::Key25519 &key,
        const chain::GroupState &groupState
    ) {
        const auto blockchain = chain::ClientBlockchain::createEmpty();
        const auto changes = makeChangesForNewState(groupState);
        if (changes.empty()) {
            return std::nullopt;
        }
        return blockchain.buildBlock(changes, key);
    }

    std::optional<bytes::binary> Session::createSelfAddBlock(
        const openssl::Key25519& key,
        const bytes::const_span previousServerBlock,
        const chain::GroupParticipant& self
    ) {
        const auto previous = fromServerToLocal(previousServerBlock);
        if (!previous) {
            return std::nullopt;
        }
        const auto blockchain = chain::ClientBlockchain::createFromBlock(bytes::view(*previous));
        if (!blockchain) {
            return std::nullopt;
        }
        auto state = blockchain->groupState();
        std::erase_if(state.participants, [&self](const chain::GroupParticipant& p) {
            return p.user_id == self.user_id;
        });
        state.participants.push_back(self);
        const auto changes = makeChangesForNewState(state);
        if (changes.empty()) {
            return std::nullopt;
        }
        return blockchain->buildBlock(changes, key);
    }

    std::optional<bytes::binary> Session::receiveInboundMessage(const bytes::const_span serverMessage) {
        std::lock_guard lock(mutex);
        const auto local = fromServerToLocal(serverMessage);
        if (!local) {
            return std::nullopt;
        }
        sessionVerification->receiveInboundMessage(bytes::view(*local));
        return sessionVerification->emojiHash();
    }

    void Session::apply(const int subchain, const bytes::binary& last) {
        if (subchain) {
            if (!blockchain) {
                failed = true;
                return;
            }
            updateEmojis(receiveInboundMessage(bytes::view(last)));
            checkForOutboundMessages();
        } else if (blockchain) {
            if (!applyBlock(bytes::view(last))) {
                failed = true;
                return;
            }
            refreshFromCall();
            checkForOutboundMessages();
        } else {
            if (!initBlockchain(bytes::view(last))) {
                failed = true;
                return;
            }
            for (auto i = 0; i != kSubChainsCount; ++i) {
                if (!subchains[i].waitingActive) {
                    scheduleShortPoll(i);
                }
            }
            refreshFromCall();
            checkForOutboundMessages();
        }
    }

    void Session::scheduleWaiting(const int subchain) {
        auto& entry = subchains[subchain];
        const auto generation = ++entry.waitingGeneration;
        entry.waitingActive = true;
        std::weak_ptr weak(shared_from_this());
        updateThread.PostDelayedTask([weak, subchain, generation] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            auto& state = strong->subchains[subchain];
            if (state.waitingGeneration != generation) {
                return;
            }
            state.waitingActive = false;
            strong->checkWaitingBlocks(subchain, true);
        }, webrtc::TimeDelta::Millis(kShortPollWaitForMs));
    }

    void Session::cancelWaiting(const int subchain) {
        auto& entry = subchains[subchain];
        ++entry.waitingGeneration;
        entry.waitingActive = false;
    }

    void Session::cancelShortPoll(const int subchain) {
        ++subchains[subchain].shortPollGeneration;
    }

    void Session::checkWaitingBlocks(const int subchain, const bool waited) {
        if (failed) {
            return;
        }

        auto& entry = subchains[subchain];
        if (!blockchain) {
            scheduleWaiting(subchain);
            return;
        }
        if (entry.shortPolling) {
            return;
        }
        auto& waiting = entry.waiting;
        cancelShortPoll(subchain);
        while (!waiting.empty()) {
            const auto index = waiting.begin()->first;
            if (index > entry.height) {
                if (waited) {
                    shortPoll(subchain);
                } else {
                    scheduleWaiting(subchain);
                }
                return;
            }
            if (index == entry.height) {
                apply(subchain, waiting.begin()->second);
                if (failed) {
                    return;
                }
                entry.height = std::max(entry.height, index + 1);
            }
            waiting.erase(waiting.begin());
        }
        cancelWaiting(subchain);
        scheduleShortPoll(subchain);
    }

    void Session::checkForOutboundMessages() {
        if (!blockchain) {
            return;
        }
        if (const auto messages = pullOutboundMessages(); !messages.empty()) {
            (void) outboundBlockCallback(messages.back());
        }
    }

    std::vector<bytes::binary> Session::pullOutboundMessages() {
        std::lock_guard lock(mutex);
        return sessionVerification->pullOutboundMessages();
    }

    bool Session::initBlockchain(const bytes::const_span serverBlock) {
        std::lock_guard lock(mutex);
        const auto local = fromServerToLocal(serverBlock);
        if (!local) {
            failed = true;
            return false;
        }
        auto created = chain::ClientBlockchain::createFromBlock(bytes::view(*local));
        if (!created) {
            failed = true;
            return false;
        }
        blockchain = std::move(*created);
        sessionVerification->onNewMainBlock(blockchain->inner());
        return updateGroupSharedKey();
    }

    bool Session::applyBlock(const bytes::const_span serverBlock) {
        std::lock_guard lock(mutex);
        const auto local = fromServerToLocal(serverBlock);
        if (!local) {
            failed = true;
            return false;
        }
        if (!blockchain->tryApplyBlock(bytes::view(*local))) {
            failed = true;
            return false;
        }
        sessionVerification->onNewMainBlock(blockchain->inner());
        return updateGroupSharedKey();
    }

    bool Session::updateGroupSharedKey() {
        sessionEncryption->forgetSharedKey(blockchain->height() - 1, blockchain->previousBlockHash());

        const auto groupState = blockchain->groupState();
        if (
            const auto self = State::findParticipant(groupState, privateKey.publicKeyBytes());
            !self || self->user_id != sessionEncryption->userId()
        ) {
            failed = true;
            return false;
        }
        auto secret = decryptSharedKey();
        if (!secret) {
            failed = true;
            return false;
        }
        if (State::groupStateVersion(groupState) >= 1) {
            const auto rehashed = openssl::Hmac::Sha512(
                bytes::view(*secret),
                bytes::view(blockchain->lastBlockHash())
            );
            secret = bytes::binary(rehashed.begin(), rehashed.begin() + 32);
        }
        return sessionEncryption->addSharedKey(blockchain->height(), blockchain->lastBlockHash(), *secret, groupState);
    }

    std::optional<bytes::binary> Session::decryptSharedKey() const {
        const auto&[
            ek,
            encryptedSharedKey,
            destUserId,
            destHeader
        ] = blockchain->groupSharedKey();
        for (size_t i = 0; i < destUserId.size(); ++i) {
            if (destUserId[i] != sessionEncryption->userId()) {
                continue;
            }
            const auto sharedSecret = privateKey.computeSharedSecret(bytes::view(ek));
            const auto oneTimeSecret = e2e::chain::MessageEncryption::decryptHeader(
                bytes::view(destHeader[i]),
                bytes::view(encryptedSharedKey),
                bytes::view(sharedSecret)
            );
            if (!oneTimeSecret) {
                return std::nullopt;
            }
            auto decrypted = e2e::chain::MessageEncryption::decryptData(
                bytes::view(encryptedSharedKey),
                bytes::view(*oneTimeSecret)
            );
            if (!decrypted || decrypted->size() != 32) {
                return std::nullopt;
            }
            return decrypted;
        }
        return std::nullopt;
    }

    void Session::scheduleShortPoll(const int subchain) {
        auto& entry = subchains[subchain];
        const auto generation = ++entry.shortPollGeneration;
        std::weak_ptr weak = weak_from_this();
        updateThread.PostDelayedTask([weak, subchain, generation] {
            const auto strong = weak.lock();
            if (!strong || strong->subchains[subchain].shortPollGeneration != generation) {
                return;
            }
            strong->shortPoll(subchain);
        }, webrtc::TimeDelta::Millis(kShortPollTimeoutMs));
    }

    chain::GroupState Session::currentGroupState() {
        std::lock_guard lock(mutex);
        return blockchain->groupState();
    }

    std::optional<bytes::binary> Session::emojiHash() {
        std::lock_guard lock(mutex);
        return sessionVerification->emojiHash();
    }

    void Session::refreshFromCall() {
        if (!blockchain) {
            return;
        }
        std::unordered_set<int64_t> userIds;
        for (const auto& participant : currentGroupState().participants) {
            userIds.insert(participant.user_id);
        }
        updateEmojis(emojiHash());
    }

    void Session::updateEmojis(const std::optional<bytes::binary>& hash) {
        if (hash) {
            fingerprintEmojis = ntgcalls::EmojiFingerprint::fromHash(bytes::view(*hash));
            (void) updateEmojisCallback(fingerprintEmojis);
        }
    }

    std::string Session::getFingerprintEmojis() {
        std::lock_guard lock(mutex);
        return fingerprintEmojis;
    }

    void Session::setLastBlock(const bytes::binary& block) {
        lastBlock = block;
    }

    bytes::binary Session::makeJoinBlock() {
        if (failed) return {};

        chain::GroupParticipant self;
        self.user_id = sessionEncryption->userId();
        self.add_users = true;
        self.remove_users = true;
        self.public_key = privateKey.publicKeyBytes();
        self.version = 0;

        std::optional<bytes::binary> block;
        if (lastBlock) {
            block = createSelfAddBlock(
                privateKey,
                bytes::view(lastBlock.value()),
                self
            );
        } else {
            chain::GroupState groupState;
            groupState.participants = {self};
            groupState.external_permissions = Permissions::AddUsers | Permissions::RemoveUsers;
            block = createZeroBlock(privateKey, groupState);
        }
        if (!block) {
            failed = true;
            return {};
        }
        return block.value();
    }

    void Session::shortPoll(const int subchain) {
        auto& entry = subchains[subchain];
        cancelWaiting(subchain);
        cancelShortPoll(subchain);
        if (subchain && !blockchain) {
            scheduleWaiting(subchain);
            return;
        }
        entry.shortPolling = true;
        (void) subchainRequestCallback(
            SubchainRequest{
                subchain,
                entry.height,
                kShortPollChainBlocksPerRequest
            }
        );
    }

    void Session::finishSubchainRequest(const int subchain) {
        if (failed) {
            return;
        }
        subchains[subchain].shortPolling = false;
        checkWaitingBlocks(subchain, false);
    }

    bytes::binary Session::publicKey() const {
        const auto publicKey = privateKey.publicKeyBytes();
        return {publicKey.begin(), publicKey.end()};
    }

    void Session::applyBlocks(
        const int subchain,
        const int nextOffset,
        const std::vector<bytes::binary> &blocks,
        const bool fromShortPoll
    ) {
        if (!subchain && !blocks.empty() && nextOffset > lastBlockHeight) {
            lastBlock = blocks.back();
            lastBlockHeight = nextOffset;
        }

        auto& entry = subchains[subchain];
        if (fromShortPoll) {
            auto i = entry.waiting.begin();
            while (i != entry.waiting.end() && i->first < nextOffset) {
                ++i;
            }
            entry.waiting.erase(entry.waiting.begin(), i);

            if (subchain && !blockchain && !blocks.empty()) {
                RTC_LOG(LS_ERROR) << "ConferenceCall: broadcast short-poll block before the call was created";
                failed = true;
                return;
            }
        } else {
            entry.lastUpdate = webrtc::TimeMillis();
        }
        if (failed) {
            return;
        }

        if (auto index = nextOffset - static_cast<int>(blocks.size()); !fromShortPoll && (index > entry.height || (!blockchain && subchain))) {
            for (const auto& block : blocks) {
                entry.waiting.emplace(index++, block);
            }
        } else {
            if (fromShortPoll && subchain && index > entry.height) {
                entry.height = index;
            }
            for (const auto& block : blocks) {
                if (!blockchain || entry.height == index) {
                    apply(subchain, block);
                }
                entry.height = std::max(entry.height, ++index);
            }
            entry.height = std::max(entry.height, nextOffset);
        }
        checkWaitingBlocks(subchain);
    }

    void Session::onOutboundBlock(const std::function<void(bytes::binary)>& callback) {
        outboundBlockCallback = callback;
    }

    void Session::onSubchainRequest(const std::function<void(SubchainRequest)> &callback) {
        subchainRequestCallback = callback;
    }

    void Session::onUpdateEmojiHash(const std::function<void(std::string)> &callback) {
        updateEmojisCallback = callback;
    }

    bytes::binary Session::encrypt(const bytes::binary& data, const size_t unencryptedPrefix) {
        std::lock_guard lock(mutex);
        if (failed) return {};
        const auto r = sessionEncryption->encrypt(0,  bytes::view(data), unencryptedPrefix);
        return r ? *r : bytes::binary();
    }

    bytes::binary Session::decrypt(const int64_t userId, const bytes::binary& data) {
        std::lock_guard lock(mutex);
        if (failed) return {};
        const auto r = sessionEncryption->decrypt(userId,  bytes::view(data));
        return r ? *r : bytes::binary();
    }
} // telegram::e2e