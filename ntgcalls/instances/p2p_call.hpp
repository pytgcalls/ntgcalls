//
// Created by Laky64 on 15/03/2024.
//

#pragma once
#include <future>
#include <nlohmann/json.hpp>
#include <call/call.h>

#include "call_interface.hpp"
#include "ntgcalls/models/auth_params.hpp"
#include "ntgcalls/signaling/signaling.hpp"
#include "../models/rtc_server.hpp"

namespace ntgcalls {
    using nlohmann::json;

    class P2PCall final: public CallInterface {
        bytes::vector randomPower, prime;
        std::optional<signaling::RawKey> key;
        std::optional<bytes::vector> g_a_hash, g_a_or_b;
        std::atomic_bool isMakingOffer = false, makingNegotation = false, handshakeCompleted = false;
        std::unique_ptr<signaling::SignalingInterface> signaling;
        wrtc::synchronized_callback<bytes::binary> onEmitData;
        std::vector<wrtc::IceCandidate> pendingIceCandidates;
        signaling::Signaling::Version protocolVersion = signaling::Signaling::Version::Unknown;

        void processSignalingData(const bytes::binary& buffer);

        void sendLocalDescription();

        void applyRemoteSdp(wrtc::Description::SdpType sdpType, const std::string& sdp);

        void applyPendingIceCandidates();

        void sendMediaState(MediaState mediaState) const;

    public:
        explicit P2PCall(rtc::Thread* workerThread): CallInterface(workerThread) {}

        bytes::vector init(int32_t g, const bytes::vector &p, const bytes::vector &r, const std::optional<bytes::vector> &g_a_hash, const MediaDescription &media);

        AuthParams exchangeKeys(const bytes::vector &g_a_or_b, int64_t fingerprint);

        void connect(const std::vector<RTCServer>& servers, const std::vector<std::string>& versions, bool p2pAllowed);

        Type type() const override;

        void onSignalingData(const std::function<void(const bytes::binary&)>& callback);

        void sendSignalingData(const bytes::binary& buffer);
    };

} // ntgcalls
