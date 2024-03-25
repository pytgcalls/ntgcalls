//
// Created by Laky64 on 15/03/2024.
//

#include "p2p_call.hpp"

#include "ntgcalls/exceptions.hpp"
#include "ntgcalls/models/candidate_message.hpp"
#include "ntgcalls/models/media_state_message.hpp"
#include "ntgcalls/models/rtc_description_message.hpp"
#include "ntgcalls/utils/auth_key.hpp"
#include "ntgcalls/utils/mod_exp_first.hpp"
#include "wrtc/utils/encryption.hpp"

namespace ntgcalls {
    bytes::vector P2PCall::init(const int32_t g, const bytes::vector &p, const bytes::vector &r, const std::optional<bytes::vector> &g_a_hash, const MediaDescription &media) {
        std::lock_guard lock(mutex);
        if (g_a_or_b) {
            throw ConnectionError("Connection already made");
        }
        auto first = ModExpFirst(g, p, r);
        if (first.modexp.empty()) {
            throw CryptoError("Invalid modexp");
        }
        randomPower = std::move(first.randomPower);
        prime = p;
        if (g_a_hash) {
            this->g_a_hash = g_a_hash;
        }
        g_a_or_b = std::move(first.modexp);
        stream->setAVStream(media);
        return g_a_hash ? g_a_or_b.value() : openssl::Sha256::Digest(g_a_or_b.value());
    }

    AuthParams P2PCall::exchangeKeys(const bytes::vector &p, const bytes::vector &g_a_or_b, const int64_t fingerprint) {
        std::lock_guard lock(mutex);
        if (connection) {
            throw ConnectionError("Connection already made");
        }
        if (!this->g_a_or_b) {
            throw ConnectionNotFound("Connection not initialized");
        }
        if (key) {
            throw ConnectionError("Key already exchanged");
        }
        if (g_a_hash) {
            if (!fingerprint) {
                throw InvalidParams("Fingerprint not found");
            }
            if (g_a_hash != openssl::Sha256::Digest(g_a_or_b)) {
                throw CryptoError("Hash mismatch");
            }
        }
        const auto computedAuthKey = AuthKey::CreateAuthKey(
            g_a_or_b,
            randomPower,
            g_a_hash ? prime:p
        );
        if (computedAuthKey.empty()) {
            throw CryptoError("Could not create auth key");
        }
        RawKey authKey;
        AuthKey::FillData(authKey, computedAuthKey);
        const auto computedFingerprint = AuthKey::Fingerprint(authKey);
        if (g_a_hash && computedFingerprint != static_cast<uint64_t>(fingerprint)) {
            throw CryptoError("Fingerprint mismatch");
        }
        key = authKey;
        return AuthParams{
            static_cast<int64_t>(computedFingerprint),
            this->g_a_or_b.value(),
        };
    }

    void P2PCall::connect(const std::vector<wrtc::RTCServer>& servers, const std::vector<std::string>& versions) {
        std::unique_lock lock(mutex);
        if (connection) {
            throw ConnectionError("Connection already made");
        }
        if (!g_a_or_b || !key) {
            throw ConnectionNotFound("Connection not initialized");
        }
        connection = std::make_unique<wrtc::PeerConnection>(servers, true);
        connection->onRenegotiationNeeded([this] {
            if (makingNegotation) {
                sendLocalDescription();
            }
        });
        connection->onDataChannelOpened([this] {
            sendMediaState(stream->getState());
        });
        connection->onIceCandidate([this](const wrtc::IceCandidate& candidate) {
            CandidateMessage message;
            message.sdp = candidate.sdp;
            message.mid = candidate.mid;
            message.mLine = candidate.mLine;
            signaling->send(message.serialize());
        });
        auto encryptionKey = std::make_shared<std::array<uint8_t, EncryptionKey::kSize>>();
        memcpy(encryptionKey->data(), key.value().data(), EncryptionKey::kSize);
        stream->addTracks(connection);
        stream->onUpgrade([this] (const MediaState mediaState) {
            sendMediaState(mediaState);
        });
        signaling = Signaling::Create(
            versions,
            connection->networkThread(),
            connection->signalingThread(),
            EncryptionKey(std::move(encryptionKey), type() == Type::Outgoing),
            [this](const bytes::binary &data) {
                (void) onEmitData(data);
            },
            [this](const std::optional<bytes::binary> &data) {
                if (data) {
                    processSignalingData(data.value());
                }
            }
        );
        if (type() == Type::Outgoing) {
            connection->createDataChannel("data");
            makingNegotation = true;
            sendLocalDescription();
        }
        std::promise<void> promise;
        connection->onConnectionChange([this, &promise](const wrtc::PeerConnectionState state) {
            switch (state) {
            case wrtc::PeerConnectionState::Connected:
                if (!connected) {
                    connected = true;
                    stream->start();
                    promise.set_value();
                }
                break;
            case wrtc::PeerConnectionState::Disconnected:
            case wrtc::PeerConnectionState::Failed:
            case wrtc::PeerConnectionState::Closed:
                connection->onConnectionChange(nullptr);
                if (!connected) {
                    promise.set_exception(std::make_exception_ptr(TelegramServerError("Error while connecting to the P2P call server")));
                } else {
                    (void) onCloseConnection();
                }
                break;
            default:
                break;
            }
        });
        lock.unlock();
        if (promise.get_future().wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
            throw TelegramServerError("Connection timeout");
        }
    }

    void P2PCall::processSignalingData(const bytes::binary& buffer) {
        if (const auto messageType = Message::type(buffer); messageType == Message::Type::RtcDescription) {
            const auto message = RtcDescriptionMessage::deserialize(buffer);
            if (type() == Type::Outgoing && message->type == wrtc::Description::SdpType::Offer && (isMakingOffer || connection->signalingState() != wrtc::SignalingState::Stable)) {
                return;
            }
            applyRemoteSdp(
                message->type,
                message->sdp
            );
        } else if (messageType == Message::Type::Candidate) {
            const auto message = CandidateMessage::deserialize(buffer);
            const auto candidate = wrtc::IceCandidate(
                message->mid,
                message->mLine,
                message->sdp
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
        connection->setLocalDescription([this] {
            connection->signalingThread()->PostTask([this] {
                const auto description = connection->localDescription();
                if (!description) {
                    return;
                }
                RtcDescriptionMessage message;
                message.type = description->type();
                message.sdp = description->sdp();
                signaling->send(message.serialize());
                isMakingOffer = false;
            });
        }, [this](const std::exception_ptr&) {});
    }

    void P2PCall::applyRemoteSdp(const wrtc::Description::SdpType sdpType, const std::string& sdp) {
        connection->setRemoteDescription(
            wrtc::Description(
                sdpType,
                sdp
            ),
            [this, sdpType] {
                connection->signalingThread()->PostTask([this, sdpType] {
                    if (sdpType == wrtc::Description::SdpType::Offer) {
                        makingNegotation = true;
                        sendLocalDescription();
                    }
                });
            },
            [this](const std::exception_ptr&) {}
        );
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

    void P2PCall::sendMediaState(const MediaState mediaState) const {
        if (!connection -> isDataChannelOpen()) {
            return;
        }
        MediaStateMessage message;
        message.isMuted = mediaState.muted;
        if (mediaState.videoStopped) {
            message.videoState = MediaStateMessage::VideoState::Inactive;
        } else if (mediaState.videoPaused) {
            message.videoState = MediaStateMessage::VideoState::Suspended;
        } else {
            message.videoState = MediaStateMessage::VideoState::Active;
        }
        connection->sendDataChannelMessage(message.serialize());
    }

    void P2PCall::onSignalingData(const std::function<void(const bytes::binary&)>& callback) {
        onEmitData = callback;
    }

    void P2PCall::sendSignalingData(const bytes::binary& buffer) {
        std::lock_guard lock(mutex);
        if (!signaling) {
            throw ConnectionError("Connection not initialized");
        }
        signaling->receive(buffer);
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