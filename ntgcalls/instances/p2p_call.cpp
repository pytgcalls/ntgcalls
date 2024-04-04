//
// Created by Laky64 on 15/03/2024.
//

#include "p2p_call.hpp"

#include "ntgcalls/exceptions.hpp"
#include "ntgcalls/signaling/crypto/mod_exp_first.hpp"
#include "ntgcalls/signaling/messages/candidate_message.hpp"
#include "ntgcalls/signaling/messages/media_state_message.hpp"
#include "ntgcalls/signaling/messages/message.hpp"
#include "ntgcalls/signaling/messages/rtc_description_message.hpp"
#include "wrtc/utils/encryption.hpp"

namespace ntgcalls {
    bytes::vector P2PCall::init(const int32_t g, const bytes::vector &p, const bytes::vector &r, const std::optional<bytes::vector> &g_a_hash, const MediaDescription &media) {
        RTC_LOG(LS_INFO) << "Initializing P2P call";
        std::lock_guard lock(mutex);
        if (g_a_or_b) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        auto first = signaling::ModExpFirst(g, p, r);
        if (first.modexp.empty()) {
            RTC_LOG(LS_ERROR) << "Invalid modexp";
            throw CryptoError("Invalid modexp");
        }
        randomPower = std::move(first.randomPower);
        prime = p;
        if (g_a_hash) {
            this->g_a_hash = g_a_hash;
        }
        g_a_or_b = std::move(first.modexp);
        RTC_LOG(LS_INFO) << "P2P call initialized";
        stream->setAVStream(media);
        RTC_LOG(LS_INFO) << "AVStream settings applied";
        return g_a_hash ? g_a_or_b.value() : openssl::Sha256::Digest(g_a_or_b.value());
    }

    AuthParams P2PCall::exchangeKeys(const bytes::vector &g_a_or_b, const int64_t fingerprint) {
        std::lock_guard lock(mutex);
        if (connection) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        if (!this->g_a_or_b) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionNotFound("Connection not initialized");
        }
        if (key) {
            RTC_LOG(LS_ERROR) << "Key already exchanged";
            throw ConnectionError("Key already exchanged");
        }
        if (g_a_hash) {
            if (!fingerprint) {
                RTC_LOG(LS_ERROR) << "Fingerprint not found";
                throw InvalidParams("Fingerprint not found");
            }
            if (g_a_hash != openssl::Sha256::Digest(g_a_or_b)) {
                RTC_LOG(LS_ERROR) << "Hash mismatch";
                throw CryptoError("Hash mismatch");
            }
        }
        const auto computedAuthKey = signaling::AuthKey::CreateAuthKey(
            g_a_or_b,
            randomPower,
            prime
        );
        if (computedAuthKey.empty()) {
            RTC_LOG(LS_ERROR) << "Could not create auth key";
            throw CryptoError("Could not create auth key");
        }
        signaling::RawKey authKey;
        signaling::AuthKey::FillData(authKey, computedAuthKey);
        const auto computedFingerprint = signaling::AuthKey::Fingerprint(authKey);
        if (g_a_hash && computedFingerprint != static_cast<uint64_t>(fingerprint)) {
            RTC_LOG(LS_ERROR) << "Fingerprint mismatch";
            throw CryptoError("Fingerprint mismatch");
        }
        key = authKey;
        RTC_LOG(LS_INFO) << "Key exchanged, fingerprint: " << computedFingerprint;
        return AuthParams{
            static_cast<int64_t>(computedFingerprint),
            this->g_a_or_b.value(),
        };
    }

    void P2PCall::connect(const std::vector<RTCServer>& servers, const std::vector<std::string>& versions, const bool p2pAllowed) {
        RTC_LOG(LS_INFO) << "Connecting to P2P call, p2pAllowed: " << (p2pAllowed ? "true" : "false");
        std::unique_lock lock(mutex);
        if (connection) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        if (!g_a_or_b || !key) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionNotFound("Connection not initialized");
        }
        auto encryptionKey = std::make_shared<std::array<uint8_t, signaling::EncryptionKey::kSize>>();
        memcpy(encryptionKey->data(), key.value().data(), signaling::EncryptionKey::kSize);
        protocolVersion = signaling::Signaling::matchVersion(versions);
        connection = std::make_unique<wrtc::PeerConnection>(
            RTCServer::toIceServers(servers),
            true
        );
        Safe<wrtc::PeerConnection>(connection)->onRenegotiationNeeded([this] {
            if (makingNegotation) {
                RTC_LOG(LS_INFO) << "Renegotiation needed";
                sendLocalDescription();
            }
        });
        signaling = signaling::Signaling::Create(
            protocolVersion,
            connection->networkThread(),
            connection->signalingThread(),
            signaling::EncryptionKey(std::move(encryptionKey), type() == Type::Outgoing),
            [this](const bytes::binary &data) {
                (void) onEmitData(data);
            },
            [this](const std::vector<bytes::binary> &data) {
                for (const auto &packet : data) {
                    processSignalingData(packet);
                }
            }
        );
        connection->onIceCandidate([this](const wrtc::IceCandidate& candidate) {
            signaling::CandidateMessage candMess;
            candMess.sdp = candidate.sdp;
            candMess.mid = candidate.mid;
            candMess.mLine = candidate.mLine;
            RTC_LOG(LS_INFO) << "Sending candidate: " << bytes::to_string(candMess.serialize());
            signaling->send(candMess.serialize());
        });
        connection->onDataChannelOpened([this] {
            sendMediaState(stream->getState());
            RTC_LOG(LS_INFO) << "Data channel opened";
        });
        stream->addTracks(connection);
        stream->onUpgrade([this] (const MediaState mediaState) {
            sendMediaState(mediaState);
        });
        if (type() == Type::Outgoing) {
            RTC_LOG(LS_INFO) << "Creating data channel";
            Safe<wrtc::PeerConnection>(connection)->createDataChannel("data");
            makingNegotation = true;
            sendLocalDescription();
        }
        std::promise<void> promise;
        connection->onConnectionChange([this, &promise](const wrtc::ConnectionState state) {
            switch (state) {
            case wrtc::ConnectionState::Connected:
                if (!connected) {
                    RTC_LOG(LS_INFO) << "Connection established";
                    connected = true;
                    stream->start();
                    RTC_LOG(LS_INFO) << "Stream started";
                    promise.set_value();
                }
                break;
            case wrtc::ConnectionState::Disconnected:
            case wrtc::ConnectionState::Failed:
            case wrtc::ConnectionState::Closed:
                workerThread->PostTask([this] {
                    connection->onConnectionChange(nullptr);
                });
                if (!connected) {
                    RTC_LOG(LS_ERROR) << "Connection failed";
                    promise.set_exception(std::make_exception_ptr(TelegramServerError("Error while connecting to the P2P call server")));
                } else {
                    RTC_LOG(LS_INFO) << "Connection closed";
                    (void) onCloseConnection();
                }
                break;
            default:
                break;
            }
        });
        lock.unlock();
        if (promise.get_future().wait_for(std::chrono::seconds(60)) != std::future_status::ready) {
            RTC_LOG(LS_ERROR) << "Connection timeout";
            throw TelegramServerError("Connection timeout");
        }
    }

    void P2PCall::processSignalingData(const bytes::binary& buffer) {
        RTC_LOG(LS_INFO) << "processSignalingData: " << std::string(buffer.begin(), buffer.end());
        try {
            switch (signaling::Message::type(buffer)) {
            case signaling::Message::Type::RtcDescription: {
                const auto message = signaling::RtcDescriptionMessage::deserialize(buffer);
                if (
                    type() == Type::Outgoing &&
                    message->type == wrtc::Description::SdpType::Offer &&
                    (isMakingOffer || Safe<wrtc::PeerConnection>(connection)->signalingState() != wrtc::SignalingState::Stable)
                ) {
                    return;
                }
                applyRemoteSdp(
                    message->type,
                    message->sdp
                );
                break;
            }
            case signaling::Message::Type::Candidate: {
                const auto message = signaling::CandidateMessage::deserialize(buffer);
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
                break;
            }
            default:
                break;
            }
        } catch (InvalidParams& e) {
            RTC_LOG(LS_ERROR) << "Invalid params: " << e.what();
        }
    }

    void P2PCall::sendLocalDescription() {
        isMakingOffer = true;
        RTC_LOG(LS_INFO) << "Calling SetLocalDescription";
        Safe<wrtc::PeerConnection>(connection)->setLocalDescription([this] {
            connection->signalingThread()->PostTask([this] {
                assert(signaling);
                const auto description = Safe<wrtc::PeerConnection>(connection)->localDescription();
                if (!description) {
                    return;
                }
                signaling::RtcDescriptionMessage message;
                message.type = description->type();
                message.sdp = description->sdp();
                RTC_LOG(LS_INFO) << "Sending local description: " << bytes::to_string(message.serialize());
                signaling->send(message.serialize());
                isMakingOffer = false;
            });
        }, [this](const std::exception_ptr&) {});
    }

    void P2PCall::applyRemoteSdp(const wrtc::Description::SdpType sdpType, const std::string& sdp) {
        RTC_LOG(LS_INFO) << "Calling SetRemoteDescription";
        Safe<wrtc::PeerConnection>(connection)->setRemoteDescription(
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
        signaling::MediaStateMessage message;
        message.isMuted = mediaState.muted;
        if (mediaState.videoStopped) {
            message.videoState = signaling::MediaStateMessage::VideoState::Inactive;
        } else if (mediaState.videoPaused) {
            message.videoState = signaling::MediaStateMessage::VideoState::Suspended;
        } else {
            message.videoState = signaling::MediaStateMessage::VideoState::Active;
        }
        RTC_LOG(LS_INFO) << "Sending media state: " << bytes::to_string(message.serialize());
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