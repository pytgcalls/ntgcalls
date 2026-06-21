//
// Created by laky64 on 11/06/26.
//

#include <ntgcalls/instances/conference_call.hpp>


namespace ntgcalls {
    ConferenceJoinParams ConferenceCall::initConference(const int64_t userID, const std::optional<bytes::binary>& lastBlock) {
        session = std::make_shared<e2e::Session>(updateThread, userID);
        auto payload = init();
        Safe<wrtc::GroupConnection>(connection)->setE2EEncryptor(session.get());
        if (lastBlock) {
            session->setLastBlock(*lastBlock);
        }
        std::weak_ptr weak(shared_from_this());
        Safe<wrtc::GroupConnection>(connection)->onRequestParticipants([weak] {
            const auto strong = std::static_pointer_cast<ConferenceCall>(weak.lock());
            if (!strong) {
                return;
            }
            (void) strong->requestParticipantsCallback();
        });
        return {
            std::move(payload),
            session->publicKey(),
            session->makeJoinBlock()
        };
    }

    void ConferenceCall::connect(const std::string &jsonData, const bool isPresentation) {
        GroupCall::connect(jsonData, isPresentation);
        session->shortPoll(0);
        session->shortPoll(1);
    }

    void ConferenceCall::migrate(const P2PCall *p2pCall) {
        streamManager = std::move(p2pCall->getStreamManager());
        streamManager->enableVideoSimulcast(true);
        streamManager->detach();
    }

    void ConferenceCall::applyBlocks(
        const int subchain,
        const int nextOffset,
        const std::vector<bytes::binary> &blocks,
        const bool fromShortPoll
    ) const {
        session->applyBlocks(subchain, nextOffset, blocks, fromShortPoll);
    }

    void ConferenceCall::finishSubchainRequest(const int subchain) const {
        session->finishSubchainRequest(subchain);
    }

    void ConferenceCall::updateAudioSsrcMappings(const std::vector<wrtc::SsrcMapping> &audioSsrcs) const {
        const auto groupConnection = Safe<wrtc::GroupConnection>(connection);
        if (!groupConnection) {
            throw ConnectionError("Conference connection not initialized");
        }
        groupConnection->updateAudioSsrcMappings(audioSsrcs);
    }

    void ConferenceCall::onOutboundBlock(const std::function<void(bytes::binary)> &callback) const {
        session->onOutboundBlock(callback);
    }

    void ConferenceCall::onSubchainRequest(const std::function<void(e2e::SubchainRequest)> &callback) const {
        session->onSubchainRequest(callback);
    }

    void ConferenceCall::onRequestParticipants(const std::function<void()> &callback) {
        requestParticipantsCallback = callback;
    }

    CallInterface::Type ConferenceCall::type() const {
        return Type::Conference;
    }

    void ConferenceCall::stop() {
        GroupCall::stop();
        session = nullptr;
    }
} // ntgcalls