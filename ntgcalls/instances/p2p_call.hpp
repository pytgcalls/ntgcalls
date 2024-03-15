//
// Created by Laky64 on 15/03/2024.
//

#pragma once
#include <nlohmann/json.hpp>

#include "call_interface.hpp"
#include "ntgcalls/models/auth_params.hpp"
#include "ntgcalls/models/rtc_server.hpp"
#include "ntgcalls/utils/signaling_connection.hpp"

namespace ntgcalls {
    using nlohmann::json;

    class P2PCall final: public CallInterface {
        bytes::binary randomPower, prime, g_a_or_b, g_a_hash;
        std::atomic_bool isMakingOffer = false, makingNegotation = false, handshakeCompleted = false;
        std::shared_ptr<SignalingConnection> signaling;
        wrtc::synchronized_callback<bytes::binary> onEmitData;

        void processSignalingData(const bytes::binary& buffer);

        void sendLocalDescription();

        void applyRemoteSdp(wrtc::Description::Type sdpType, const std::string& sdp);

    public:
        bytes::binary init(int32_t g, const bytes::binary& p, const bytes::binary& r, const bytes::binary& g_a_hash);

        AuthParams confirmConnection(const bytes::binary& p, const bytes::binary& g_a_or_b, const int64_t& fingerprint, const std::vector<RTCServer>& servers, const std::vector<std::string> &versions);

        Type type() const override;

        void onSignalingData(const std::function<void(bytes::binary)> &callback);

        void sendSignalingData(const bytes::binary& buffer) const;
    };

} // ntgcalls
