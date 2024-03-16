//
// Created by Laky64 on 15/03/2024.
//

#include "p2p_call.hpp"

#include <iostream>

#include "ntgcalls/exceptions.hpp"
#include "ntgcalls/utils/auth_key.hpp"
#include "ntgcalls/utils/mod_exp_first.hpp"
#include "wrtc/utils/encryption.hpp"

namespace ntgcalls {
    bytes::vector P2PCall::init(const int32_t g, const bytes::vector &p, const bytes::vector &r, const std::optional<bytes::vector> &g_a_hash) {
        if (!g_a_or_b.empty()) {
            throw ConnectionError("Connection already made");
        }
        auto first = ModExpFirst(g, p, r);
        if (first.modexp.empty()) {
            throw InvalidParams("Invalid modexp");
        }
        randomPower = std::move(first.randomPower);
        prime = p;
        if (!g_a_hash) {
            this->g_a_hash = g_a_hash;
        }
        g_a_or_b = std::move(first.modexp);
        return g_a_hash ? g_a_or_b : openssl::Sha256::Digest(g_a_or_b);
    }

    AuthParams P2PCall::confirmConnection(const bytes::vector &p, const bytes::vector &g_a_or_b, const int64_t fingerprint, const std::vector<wrtc::RTCServer>& servers, const std::vector<std::string>& versions) {
        if (connection) {
            throw ConnectionError("Connection already made");
        }
        if (this->g_a_or_b.empty()) {
            throw ConnectionError("Connection not initialized");
        }
        if (g_a_hash) {
            if (!fingerprint) {
                throw InvalidParams("Fingerprint not found");
            }
            if (g_a_hash != openssl::Sha256::Digest(g_a_or_b)) {
                throw InvalidParams("Hash mismatch");
            }
        }
        const auto computedAuthKey = AuthKey::CreateAuthKey(
            g_a_or_b,
            randomPower,
            g_a_hash ? prime:p
        );
        if (computedAuthKey.empty()) {
            throw ConnectionError("Could not create auth key");
        }
        RawKey authKey;
        AuthKey::FillData(authKey, computedAuthKey);
        const auto computedFingerprint = AuthKey::Fingerprint(authKey);
        if (g_a_hash && computedFingerprint != static_cast<uint64_t>(fingerprint)) {
            throw InvalidParams("Fingerprint mismatch");
        }
        connection = std::make_shared<wrtc::PeerConnection>(servers);
        connection->onRenegotiationNeeded([this] {
            std::lock_guard lock(mutex);
            if (makingNegotation) {
                sendLocalDescription();
            }
        });
        connection->onIceCandidate([this](const wrtc::IceCandidate& candidate) {
            const json packets = {
                {"@type", "candidate"},
                {"sdp", candidate.sdp},
                {"mid", candidate.mid},
                {"mline", candidate.mLine},
            };
            std::cout << "Sending ice candidate" << std::endl;
            signaling->send(bytes::make_binary(to_string(packets)));
            std::cout << "Sent ice candidate: " << packets << std::endl;
        });
        connection->onIceStateChange([&](const wrtc::IceState state) {
            switch (state) {
            case wrtc::IceState::Connected:
                std::cout << "Connected" << std::endl;
                break;
            case wrtc::IceState::Checking:
                std::cout << "Checking" << std::endl;
                break;
            case wrtc::IceState::Completed:
                std::cout << "Completed" << std::endl;
                break;
            case wrtc::IceState::New:
                std::cout << "New" << std::endl;
                break;
            case wrtc::IceState::Disconnected:
            case wrtc::IceState::Failed:
            case wrtc::IceState::Closed:
                std::cout << "Disconnected" << std::endl;
                break;
            default:
                break;
            }
        });
        const auto encryptionKey = std::make_shared<std::array<uint8_t, kSize>>();
        memcpy(encryptionKey->data(), authKey.data(), kSize);
        stream->addTracks(connection);
        signaling = Signaling::Create(
            versions,
            connection->networkThread(),
            connection->signalingThread(),
            type() == Type::Outgoing,
            encryptionKey,
            [this](const bytes::binary &data) {
                (void) this->onEmitData(data);
            },
            [this](const std::optional<bytes::binary> &data) {
                std::lock_guard lock(mutex);
                if (data) {
                    processSignalingData(data.value());
                }
            }
        );
        if (type() == Type::Outgoing) {
            std::lock_guard lock(mutex);
            makingNegotation = true;
            sendLocalDescription();
        }
        return {
            computedFingerprint,
            this->g_a_or_b,
        };
    }

    void P2PCall::processSignalingData(const bytes::binary& buffer) {
        json data = json::parse(buffer.begin(), buffer.end());
        std::cout << "Signaling data: " << data << std::endl;
        if (data["@type"].is_null()) {
            return;
        }
        if (const auto sdpType = data["@type"]; sdpType == "offer" || sdpType == "answer") {
            const auto jsonSdp = data["sdp"];
            if (jsonSdp.is_null()) {
                return;
            }
            if (type() == Type::Outgoing && sdpType == "offer" && (isMakingOffer || connection->signalingState() != wrtc::SignalingState::Stable)) {
                return;
            }
            applyRemoteSdp(
                wrtc::Description::parseType(sdpType),
                jsonSdp
            );
        } else if (sdpType == "candidate") {
            const auto jsonMid = data["mid"];
            if (jsonMid.is_null()) {
                return;
            }
            const auto jsonMLine = data["mline"];
            if (jsonMLine.is_null()) {
                return;
            }
            const auto jsonSdp = data["sdp"];
            if (jsonSdp.is_null()) {
                return;
            }
            const auto candidate = wrtc::IceCandidate(
                jsonMid,
                jsonMLine,
                jsonSdp
            );
            if (handshakeCompleted) {
                connection->addIceCandidate(candidate);
            } else {
                pendingIceCandidates.push_back(candidate);
            }
        }
    }

    void P2PCall::sendLocalDescription() {
        isMakingOffer = true;
        connection->setLocalDescription(std::nullopt, [this]{
            connection->signalingThread()->PostTask([this] {
                std::lock_guard lock(mutex);
                const auto description = connection->localDescription();
                if (!description) {
                    return;
                }
                const json packets = {
                    {"@type", wrtc::Description::typeToString(description->getType())},
                    {"sdp", description->getSdp()}
                };
                signaling->send(bytes::make_binary(to_string(packets)));
                isMakingOffer = false;
            });
        });
    }

    void P2PCall::applyRemoteSdp(const wrtc::Description::Type sdpType, const std::string& sdp) {
        connection->setRemoteDescription(wrtc::Description(
            sdpType,
            sdp
        ), [this, sdpType] {
            connection->signalingThread()->PostTask([this, sdpType] {
                if (sdpType == wrtc::Description::Type::Offer) {
                    std::lock_guard lock(mutex);
                    makingNegotation = true;
                    sendLocalDescription();
                }
            });
        });
        if (!handshakeCompleted) {
            handshakeCompleted = true;
            applyPendingIceCandidates();
        }
    }

    void P2PCall::applyPendingIceCandidates() {
        if (pendingIceCandidates.empty()) {
            return;
        }
        for (const auto& candidate : pendingIceCandidates) {
            connection->addIceCandidate(candidate);
        }
        pendingIceCandidates.clear();
    }

    void P2PCall::onSignalingData(const std::function<void(const bytes::binary&)>& callback) {
        this->onEmitData = callback;
    }

    void P2PCall::sendSignalingData(const bytes::binary& buffer) const {
        if (signaling) {
            signaling->receive(buffer);
        }
    }

    CallInterface::Type P2PCall::type() const {
        if (!g_a_or_b.empty()) {
            if (g_a_hash) {
                return Type::Incoming;
            }
            return Type::Outgoing;
        }
        return Type::P2P;
    }
} // ntgcalls