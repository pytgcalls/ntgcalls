//
// Created by Laky64 on 15/03/2024.
//

#include "p2p_call.hpp"

#include <iostream>

#include "ntgcalls/exceptions.hpp"
#include "ntgcalls/utils/auth_key.hpp"
#include "ntgcalls/utils/mod_exp_first.hpp"

namespace ntgcalls {
    bytes::binary P2PCall::init(int32_t g, const bytes::binary& p, const bytes::binary& r,const bytes::binary& g_a_hash) {
        if (g_a_or_b) {
            throw ConnectionError("Connection already made");
        }
        auto first = ModExpFirst(g, p, r);
        if (!first.modexp) {
            throw InvalidParams("Invalid modexp");
        }
        randomPower = std::move(first.randomPower);
        prime = p;
        if (g_a_hash) {
            this->g_a_hash = g_a_hash;
        }
        g_a_or_b = std::move(first.modexp);
        return g_a_hash ? g_a_or_b:g_a_or_b.Sha256();
    }

    AuthParams P2PCall::confirmConnection(const bytes::binary& p, const bytes::binary& g_a_or_b, const int64_t& fingerprint, const std::vector<RTCServer>& servers, const std::vector<std::string> &versions) {
        if (connection) {
            throw ConnectionError("Connection already made");
        }
        if (!this->g_a_or_b) {
            throw ConnectionError("Connection not initialized");
        }
        if (g_a_hash) {
            if (!fingerprint) {
                throw InvalidParams("Fingerprint not found");
            }
            if (g_a_hash != g_a_or_b.Sha256()) {
                throw InvalidParams("Hash mismatch");
            }
        }
        const auto computedAuthKey = AuthKey::CreateAuthKey(
            g_a_or_b,
            randomPower,
            g_a_hash ? prime:p
        );
        if (!computedAuthKey) {
            throw ConnectionError("Could not create auth key");
        }
        auto authKey = AuthKey::FillData(computedAuthKey);
        const auto computedFingerprint = AuthKey::Fingerprint(authKey);
        if (g_a_hash && computedFingerprint != fingerprint) {
            throw InvalidParams("Fingerprint mismatch");
        }
        connection = std::make_shared<wrtc::PeerConnection>(RTCServer::toIceServers(servers));
        stream->addTracks(connection);
        signaling = std::make_shared<SignalingConnection>(
            versions,
            connection->networkThread(),
            type() == Type::Outgoing,
            authKey,
            [this](const bytes::binary& data) {
                (void) this->onEmitData(data);
            },
            [this](const bytes::binary& data) {
                this->processSignalingData(data);
            }
        );
        if (type() == Type::Outgoing) {
            makingNegotation = true;
            sendLocalDescription();
        }
        return {
            computedFingerprint,
            this->g_a_or_b,
        };
    }

    void P2PCall::processSignalingData(const bytes::binary& buffer) {
        if (signaling) {
            json data = json::parse(std::string(static_cast<char*>(buffer), buffer.size()));
            std::cout << "Signaling data: " << data << std::endl;
            if (data["@type"].is_null()) {
                return;
            }
            if (const auto sdpType = data["@type"]; sdpType == "offer" || sdpType == "answer") {
                const auto jsonSdp = data["sdp"];
                if (jsonSdp.is_null()) {
                    std::cout << "Invalid sdp" << std::endl;
                    return;
                }
                if (const bool offerCollision = sdpType == "offer" && (isMakingOffer || connection->signalingState() != wrtc::SignalingState::Stable); type() == Type::Outgoing && offerCollision) {
                    std::cout << "Offer collision" << std::endl;
                    return;
                }
                std::cout << "Applying remote sdp" << std::endl;
                applyRemoteSdp(
                    wrtc::Description::parseType(sdpType),
                    jsonSdp
                );
            }
        }
    }

    void P2PCall::sendLocalDescription() {
        isMakingOffer = true;
        connection->setLocalDescription();
        const auto description = connection->localDescription();
        const json packets = {
            {"@type", description->getType() == wrtc::Description::Type::Offer ? "offer":"answer"},
            {"sdp", description->getSdp()}
        };
        signaling->send(bytes::binary(to_string(packets)));
        isMakingOffer = false;
    }

    void P2PCall::applyRemoteSdp(const wrtc::Description::Type sdpType, const std::string& sdp) {
        connection->setRemoteDescription(wrtc::Description(
            sdpType,
            sdp
        ));
        if (sdpType == wrtc::Description::Type::Offer) {
            makingNegotation = true;
            sendLocalDescription();
        }
        if (!handshakeCompleted) {
            handshakeCompleted = true;
            //commitPendingIceCandidates();
        }
    }

    void P2PCall::onSignalingData(const std::function<void(bytes::binary)>& callback) {
        this->onEmitData = callback;
    }

    void P2PCall::sendSignalingData(const bytes::binary& buffer) const {
        if (signaling) {
            signaling->receive(buffer);
        }
    }

    CallInterface::Type P2PCall::type() const {
        if (g_a_or_b) {
            if (g_a_hash) {
                return Type::Incoming;
            }
            return Type::Outgoing;
        }
        return Type::P2P;
    }
} // ntgcalls