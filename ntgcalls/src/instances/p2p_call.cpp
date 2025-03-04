//
// Created by Laky64 on 15/03/2024.
//

#include <ntgcalls/instances/p2p_call.hpp>

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/signaling/crypto/mod_exp_first.hpp>
#include <ntgcalls/signaling/messages/candidates_message.hpp>
#include <ntgcalls/signaling/messages/candidate_message.hpp>
#include <ntgcalls/signaling/messages/initial_setup_message.hpp>
#include <ntgcalls/signaling/messages/media_state_message.hpp>
#include <ntgcalls/signaling/messages/message.hpp>
#include <ntgcalls/signaling/messages/negotiate_channels_message.hpp>
#include <ntgcalls/signaling/messages/rtc_description_message.hpp>
#include <wrtc/interfaces/native_connection.hpp>
#include <wrtc/utils/encryption.hpp>

namespace ntgcalls {
    P2PCall::~P2PCall() {
        signaling = nullptr;
    }

    void P2PCall::init(const MediaDescription &media) const {
        RTC_LOG(LS_INFO) << "Initializing P2P call";
        streamManager->enableVideoSimulcast(false);
        streamManager->setStreamSources(StreamManager::Mode::Capture, media);
        streamManager->setStreamSources(StreamManager::Mode::Playback, MediaDescription());
        RTC_LOG(LS_INFO) << "AVStream settings applied";
    }

    bytes::vector P2PCall::initExchange(const DhConfig& dhConfig, const std::optional<bytes::vector>& g_a_hash) {
        if (g_a_or_b) {
            RTC_LOG(LS_ERROR) << "Exchange already initialized";
            throw ConnectionError("Exchange already initialized");
        }
        auto first = signaling::ModExpFirst(dhConfig.g, dhConfig.p, dhConfig.random);
        if (first.modexp.empty()) {
            RTC_LOG(LS_ERROR) << "Invalid modexp";
            throw CryptoError("Invalid modexp");
        }
        randomPower = std::move(first.randomPower);
        prime = dhConfig.p;
        if (g_a_hash) {
            this->g_a_hash = g_a_hash;
        }
        g_a_or_b = std::move(first.modexp);
        RTC_LOG(LS_INFO) << "P2P call initialized";
        return g_a_hash ? g_a_or_b.value() : openssl::Sha256::Digest(g_a_or_b.value());
    }

    AuthParams P2PCall::exchangeKeys(const bytes::vector &g_a_or_b, const int64_t fingerprint) {
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

    void P2PCall::skipExchange(bytes::vector encryptionKey, const bool isOutgoing) {
        if (connection) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        if (!skipExchangeKey.empty()) {
            RTC_LOG(LS_ERROR) << "Key already exchanged";
            throw ConnectionError("Key already exchanged");
        }
        skipExchangeKey = std::move(encryptionKey);
        skipIsOutgoing = isOutgoing;
        RTC_LOG(LS_INFO) << "Exchange skipped";
    }

    void P2PCall::connect(const std::vector<RTCServer>& servers, const std::vector<std::string>& versions, const bool p2pAllowed) {
        RTC_LOG(LS_INFO) << "Connecting to P2P call, p2pAllowed: " << (p2pAllowed ? "true" : "false");
        if (connection) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        auto encryptionKey = std::make_shared<std::array<uint8_t, signaling::EncryptionKey::kSize>>();
        if (skipExchangeKey.empty()) {
            if (!g_a_or_b || !key) {
                RTC_LOG(LS_ERROR) << "Connection not initialized";
                throw ConnectionNotFound("Connection not initialized");
            }
            memcpy(encryptionKey->data(), key.value().data(), signaling::EncryptionKey::kSize);
        } else {
            memcpy(encryptionKey->data(), skipExchangeKey.data(), signaling::EncryptionKey::kSize);
        }
        protocolVersion = signaling::Signaling::matchVersion(versions);
        if (protocolVersion & signaling::Signaling::Version::V2Full) {
            connection = std::make_shared<wrtc::PeerConnection>(
                RTCServer::toIceServers(servers),
                true,
            p2pAllowed
            );
            Safe<wrtc::PeerConnection>(connection)->onRenegotiationNeeded([this] {
                if (makingNegotation) {
                    RTC_LOG(LS_INFO) << "Renegotiation needed";
                    sendLocalDescription();
                }
            });
        } else {
            connection = std::make_shared<wrtc::NativeConnection>(
                RTCServer::toRtcServers(servers),
                p2pAllowed,
                type() == Type::Outgoing
            );
        }
        connection->open();
        streamManager->optimizeSources(connection.get());
        signaling = signaling::Signaling::Create(
            protocolVersion,
            connection->networkThread(),
            connection->signalingThread(),
            connection->environment(),
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
        std::weak_ptr weakSignaling = signaling;
        connection->onIceCandidate([this, weakSignaling](const wrtc::IceCandidate& candidate) {
            const auto signaling = weakSignaling.lock();
            if (!signaling) {
                return;
            }
            bytes::binary message;
            if (protocolVersion & signaling::Signaling::Version::V2Full) {
                signaling::CandidateMessage candMess;
                candMess.sdp = candidate.sdp;
                candMess.mid = candidate.mid;
                candMess.mLine = candidate.mLine;
                message = candMess.serialize();
            } else {
                signaling::CandidatesMessage candMess;
                candMess.iceCandidates.push_back({candidate.sdp});
                message = candMess.serialize();
            }
            RTC_LOG(LS_INFO) << "Sending candidate: " << bytes::to_string(message);
            signaling->send(message);
        });
        connection->onDataChannelOpened([this] {
            sendMediaState(streamManager->getState());
            RTC_LOG(LS_INFO) << "Data channel opened";
        });
        connection->onDataChannelMessage([this](const bytes::binary& data) {
            processSignalingData(data);
        });
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Microphone, connection.get());
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Camera, connection.get());
        streamManager->addTrack(StreamManager::Mode::Playback, StreamManager::Device::Microphone, connection.get());
        streamManager->addTrack(StreamManager::Mode::Playback, StreamManager::Device::Camera, connection.get());
        streamManager->onUpgrade([this] (const MediaState mediaState) {
            sendMediaState(mediaState);
        });
        if (type() == Type::Outgoing) {
            if (protocolVersion & signaling::Signaling::Version::V2Full) {
                RTC_LOG(LS_INFO) << "Creating data channel";
                Safe<wrtc::PeerConnection>(connection)->createDataChannel("data");
                makingNegotation = true;
                sendLocalDescription();
            } else {
                sendInitialSetup();
                sendOfferIfNeeded();
            }
        }
        setConnectionObserver(connection);
    }

    void P2PCall::processSignalingData(const bytes::binary& buffer) {
        if (signaling == nullptr) {
            return;
        }
        RTC_LOG(LS_INFO) << "processSignalingData: " << std::string(buffer.begin(), buffer.end());
        try {
            switch (signaling::Message::type(buffer)) {
            case signaling::Message::Type::InitialSetup: {
                const auto message = signaling::InitialSetupMessage::deserialize(buffer);
                wrtc::PeerIceParameters remoteIceParameters;
                remoteIceParameters.ufrag = message->ufrag;
                remoteIceParameters.pwd = message->pwd;
                remoteIceParameters.supportsRenomination = message->supportsRenomination;

                std::unique_ptr<rtc::SSLFingerprint> fingerprint;
                std::string sslSetup;
                if (!message->fingerprints.empty()) {
                    fingerprint = rtc::SSLFingerprint::CreateUniqueFromRfc4572(message->fingerprints[0].hash, message->fingerprints[0].fingerprint);
                    sslSetup = message->fingerprints[0].setup;
                }
                Safe<wrtc::NativeConnection>(connection)->setRemoteParams(remoteIceParameters, std::move(fingerprint), sslSetup);
                handshakeCompleted = true;
                if (type() == Type::Incoming) {
                    sendInitialSetup();
                }
                applyPendingIceCandidates();
                break;
            }
            case signaling::Message::Type::Candidates: {
                for (const auto message = signaling::CandidatesMessage::deserialize(buffer); const auto&[sdpString] : message->iceCandidates) {
                    webrtc::JsepIceCandidate parseCandidate{ std::string(), 0 };
                    if (!parseCandidate.Initialize(sdpString, nullptr)) {
                        RTC_LOG(LS_ERROR) << "Could not parse candidate: " << sdpString;
                        continue;
                    }
                    std::string sdp;
                    parseCandidate.ToString(&sdp);
                    pendingIceCandidates.emplace_back(
                        parseCandidate.sdp_mid(),
                        parseCandidate.sdp_mline_index(),
                        sdp
                    );
                }
                if (handshakeCompleted) {
                    applyPendingIceCandidates();
                }
                break;
            }
            case signaling::Message::Type::NegotiateChannels: {
                const auto message = signaling::NegotiateChannelsMessage::deserialize(buffer);
                auto negotiationContents = std::make_unique<wrtc::ContentNegotiationContext::NegotiationContents>();
                negotiationContents->exchangeId = message->exchangeId;
                negotiationContents->contents = message->contents;
                auto negotiation = message->serialize();
                if (const auto response = Safe<wrtc::NativeConnection>(connection)->setPendingAnswer(std::move(negotiationContents))) {
                    signaling::NegotiateChannelsMessage channelMessage;
                    channelMessage.exchangeId = response->exchangeId;
                    channelMessage.contents = response->contents;
                    RTC_LOG(LS_INFO) << "Sending negotiate channels: " << bytes::to_string(channelMessage.serialize());
                    signaling->send(channelMessage.serialize());
                }
                sendOfferIfNeeded();
                Safe<wrtc::NativeConnection>(connection)->createChannels();
                break;
            }
            case signaling::Message::Type::RtcDescription: {
                const auto message = signaling::RtcDescriptionMessage::deserialize(buffer);
                if (
                    type() == Type::Outgoing &&
                    message->type == webrtc::SdpType::kOffer &&
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
            case signaling::Message::Type::MediaState: {
                const auto message = signaling::MediaStateMessage::deserialize(buffer);
                const auto cameraState = parseVideoState(message->videoState);
                const auto screenState = parseVideoState(message->screencastState);
                const auto micState = message->isMuted ? RemoteSource::State::Inactive : RemoteSource::State::Active;
                if (lastCameraState != cameraState) {
                    lastCameraState = cameraState;
                    (void) remoteSourceCallback({0, cameraState, StreamManager::Device::Camera});
                }
                if (lastScreenState != screenState) {
                    lastScreenState = screenState;
                    (void) remoteSourceCallback({0, screenState, StreamManager::Device::Screen});
                }
                if (lastMicState != micState) {
                    lastMicState = micState;
                    (void) remoteSourceCallback({0, micState, StreamManager::Device::Microphone});
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

    void P2PCall::applyRemoteSdp(const webrtc::SdpType sdpType, const std::string& sdp) {
        RTC_LOG(LS_INFO) << "Calling SetRemoteDescription";
        Safe<wrtc::PeerConnection>(connection)->setRemoteDescription(
            wrtc::Description(
                sdpType,
                sdp
            ),
            [this, sdpType] {
                connection->signalingThread()->PostTask([this, sdpType] {
                    if (sdpType == webrtc::SdpType::kOffer) {
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

        if (!streamManager->hasDevice(StreamManager::Capture, StreamManager::Camera)) {
            message.videoState = signaling::MediaStateMessage::VideoState::Inactive;
        } else if (mediaState.videoPaused) {
            message.videoState = signaling::MediaStateMessage::VideoState::Suspended;
        } else {
            message.videoState = signaling::MediaStateMessage::VideoState::Active;
        }

        if (!streamManager->hasDevice(StreamManager::Capture, StreamManager::Screen)) {
            message.screencastState = signaling::MediaStateMessage::VideoState::Inactive;
        } else if (mediaState.presentationPaused) {
            message.screencastState = signaling::MediaStateMessage::VideoState::Suspended;
        } else {
            message.screencastState = signaling::MediaStateMessage::VideoState::Active;
        }

        RTC_LOG(LS_INFO) << "Sending media state: " << bytes::to_string(message.serialize());
        connection->sendDataChannelMessage(message.serialize());
    }

    void P2PCall::sendOfferIfNeeded() const {
        if (const auto offer = Safe<wrtc::NativeConnection>(connection)->getPendingOffer()) {
            signaling::NegotiateChannelsMessage data;
            data.exchangeId = offer->exchangeId;
            data.contents = offer->contents;
            RTC_LOG(LS_INFO) << "Sending offer: " << bytes::to_string(data.serialize());
            signaling->send(data.serialize());
        }
    }

    void P2PCall::sendInitialSetup() const {
        connection->networkThread()->PostTask([this] {
            const auto localFingerprint = Safe<wrtc::NativeConnection>(connection)->localFingerprint();
            std::string hash;
            std::string fingerprint;
            if (localFingerprint) {
                hash = localFingerprint->algorithm;
                fingerprint = localFingerprint->GetRfc4572Fingerprint();
            }
            std::string setup;
            if (type() == Type::Outgoing) {
                setup = "actpass";
            } else {
                setup = "passive";
            }
            const auto localIceParams = Safe<wrtc::NativeConnection>(connection)->localIceParameters();
            connection->signalingThread()->PostTask([this, localIceParams, hash, fingerprint, setup] {
                signaling::InitialSetupMessage message;
                message.ufrag = localIceParams.ufrag;
                message.pwd = localIceParams.pwd;
                message.supportsRenomination = localIceParams.supportsRenomination;
                signaling::InitialSetupMessage::DtlsFingerprint dtlsFingerprint;
                dtlsFingerprint.hash = hash;
                dtlsFingerprint.fingerprint = fingerprint;
                dtlsFingerprint.setup = setup;
                message.fingerprints.push_back(std::move(dtlsFingerprint));
                RTC_LOG(LS_INFO) << "Sending initial setup: " << bytes::to_string(message.serialize());
                signaling->send(message.serialize());
            });
        });
    }

    void P2PCall::onSignalingData(const std::function<void(const bytes::binary&)>& callback) {
        onEmitData = callback;
    }

    void P2PCall::sendSignalingData(const bytes::binary& buffer) const {
        if (!signaling) {
            throw ConnectionError("Connection not initialized");
        }
        signaling->receive(buffer);
    }

    CallInterface::Type P2PCall::type() const {
        if (skipExchangeKey.empty()) {
            if (g_a_or_b) {
                if (g_a_hash) {
                    return Type::Incoming;
                }
                return Type::Outgoing;
            }
        } else {
            return skipIsOutgoing ? Type::Outgoing : Type::Incoming;
        }
        return Type::P2P;
    }
} // ntgcalls