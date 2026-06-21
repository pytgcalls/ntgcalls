//
// Created by laky64 on 11/06/26.
//

#pragma once
#include <ntgcalls/e2e/session.hpp>
#include <ntgcalls/instances/group_call.hpp>
#include <ntgcalls/models/conference_join_params.hpp>

namespace ntgcalls {
    using namespace telegram;

    class ConferenceCall final : public GroupCall {
        std::shared_ptr<e2e::Session> session;
        wrtc::synchronized_callback<void()> requestParticipantsCallback;

    public:
        explicit ConferenceCall(wrtc::SafeThread& updateThread): GroupCall(updateThread) {}

        ConferenceJoinParams initConference(int64_t userID, const std::optional<bytes::binary>& lastBlock);

        void connect(const std::string& jsonData, bool isPresentation) override;

        void migrate(const P2PCall *p2pCall);

        void applyBlocks(
            int subchain,
            int nextOffset,
            const std::vector<bytes::binary>& blocks,
            bool fromShortPoll
        ) const;

        void updateAudioSsrcMappings(const std::vector<wrtc::SsrcMapping> &audioSsrcs) const;

        void finishSubchainRequest(int subchain) const;

        void onOutboundBlock(const std::function<void(bytes::binary)>& callback) const;

        void onSubchainRequest(const std::function<void(e2e::SubchainRequest)> &callback) const;

        void onRequestParticipants(const std::function<void()> &callback);

        Type type() const override;

        void stop() override;
    };
} // ntgcalls
