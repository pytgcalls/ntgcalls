//
// Created by Laky64 on 29/03/2024.
//

#include <wrtc/interfaces/native_connection.hpp>

#include <memory>
#include <utility>

#include <wrtc/interfaces/reflector_relay_port_factory.hpp>
#include <wrtc/exceptions.hpp>


namespace wrtc {
    NativeConnection::NativeConnection(std::vector<RTCServer> rtcServers, const bool enableP2P, const bool isOutgoing):
    isOutgoing(isOutgoing),
    enableP2P(enableP2P),
    rtcServers(std::move(rtcServers)),
    eventLog(std::make_unique<webrtc::RtcEventLogNull>()) {}

    void NativeConnection::open() {
        initConnection();
        contentNegotiationContext = std::make_unique<ContentNegotiationContext>(factory->fieldTrials(), isOutgoing, factory->mediaEngine(), factory->ssrcGenerator(), call->GetPayloadTypeSuggester());
        contentNegotiationContext->copyCodecsFromChannelManager(factory->mediaEngine(), false);
        std::weak_ptr weak(shared_from_this());
        networkThread()->PostTask([weak] {

            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->start();
        });
    }

    cricket::RelayPortFactoryInterface* NativeConnection::getRelayPortFactory() {
        bool standaloneReflectorMode = getCustomParameterBool("network_standalone_reflectors");
        uint32_t standaloneReflectorRoleId = 0;
        if (standaloneReflectorMode) {
            if (isOutgoing) {
                standaloneReflectorRoleId = 1;
            } else {
                standaloneReflectorRoleId = 2;
            }
        }
        relayPortFactory = std::make_unique<ReflectorRelayPortFactory>(rtcServers, standaloneReflectorMode, standaloneReflectorRoleId);
        return relayPortFactory.get();
    }

    std::pair<cricket::ServerAddresses, std::vector<cricket::RelayServerConfig>> NativeConnection::getStunAndTurnServers() {
        cricket::ServerAddresses stunServers;
        std::vector<cricket::RelayServerConfig> turnServers;
        for (auto &[id, host, port, login, password, isTurn, isTcp] : rtcServers) {
            if (isTcp) {
                continue;
            }
            if (isTurn) {
                turnServers.emplace_back(
                    rtc::SocketAddress(host, port),
                    login,
                    password,
                    cricket::PROTO_UDP
                );
            } else {
                auto stunAddress = rtc::SocketAddress(host, port);
                stunServers.insert(stunAddress);
            }
        }
        return {stunServers, turnServers};
    }

    void NativeConnection::setPortAllocatorFlags(cricket::BasicPortAllocator* portAllocator) {
        uint32_t flags = portAllocator->flags();
        if (getCustomParameterBool("network_use_default_route")) {
            flags |= cricket::PORTALLOCATOR_DISABLE_ADAPTER_ENUMERATION;
        }

        if (getCustomParameterBool("network_enable_shared_socket")) {
            flags |= cricket::PORTALLOCATOR_ENABLE_SHARED_SOCKET;
        }
        flags |= cricket::PORTALLOCATOR_ENABLE_IPV6;
        flags |= cricket::PORTALLOCATOR_ENABLE_IPV6_ON_WIFI;
        flags |= cricket::PORTALLOCATOR_DISABLE_TCP;
        if (!enableP2P) {
            flags |= cricket::PORTALLOCATOR_DISABLE_UDP;
            flags |= cricket::PORTALLOCATOR_DISABLE_STUN;
            uint32_t candidateFilter = portAllocator->candidate_filter();
            candidateFilter &= ~cricket::CF_REFLEXIVE;
            portAllocator->SetCandidateFilter(candidateFilter);
        }
        portAllocator->set_step_delay(cricket::kMinimumStepDelay);
        portAllocator->set_flags(flags);
    }

    int NativeConnection::getRegatherOnFailedNetworksInterval() {
        return 8000;
    }

    cricket::IceRole NativeConnection::iceRole() const {
        return isOutgoing ? cricket::ICEROLE_CONTROLLING : cricket::ICEROLE_CONTROLLED;
    }

    cricket::IceMode NativeConnection::iceMode() const {
        return cricket::ICEMODE_FULL;
    }

    void NativeConnection::registerTransportCallbacks(cricket::P2PTransportChannel* transportChannel) {
        transportChannel->SignalCandidateGathered.connect(this, &NativeConnection::candidateGathered);
        std::weak_ptr weak(shared_from_this());
        transportChannel->SetCandidatePairChangeCallback([weak](cricket::CandidatePairChangeEvent const &event) {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->candidatePairChanged(event);
        });
        transportChannel->SignalNetworkRouteChanged.connect(this, &NativeConnection::transportRouteChanged);
    }

    std::optional<rtc::SSLRole> NativeConnection::dtlsRole() const {
        return std::nullopt;
    }

    bool NativeConnection::supportsRenomination() const {
        return localParameters.supportsRenomination;
    }

    void NativeConnection::stateUpdated(const bool isConnected) {
        if (!isConnected) {
            lastDisconnectedTimestamp = rtc::TimeMillis();
        }
        notifyStateUpdated();
    }

    int NativeConnection::candidatePoolSize() const {
        return 0;
    }

    bool NativeConnection::getCustomParameterBool(const std::string& name) const {
        if (customParameters == nullptr) {
            return false;
        }
        return customParameters[name].is_boolean() && customParameters[name];
    }

    CandidateDescription NativeConnection::connectionDescriptionFromCandidate(const cricket::Candidate& candidate) {
        CandidateDescription result;
        result.type = candidate.type_name();
        result.protocol = candidate.protocol();
        result.address = candidate.address().ToString();
        return result;
    }

    void NativeConnection::createChannels() {
        const auto coordinatedState = contentNegotiationContext->coordinatedState();
        if (!coordinatedState) {
            return;
        }
        if (audioChannelId) {
            if (const auto audioSsrc = contentNegotiationContext->outgoingChannelSsrc(*audioChannelId)) {
                if (audioChannel && audioChannel->ssrc() != audioSsrc.value()) {
                    audioChannel = nullptr;
                }
                std::optional<MediaContent> audioContent;
                for (const auto &content : coordinatedState->outgoingContents) {
                    if (content.type == MediaContent::Type::Audio && content.ssrc == audioSsrc.value()) {
                        audioContent = content;
                        break;
                    }
                }
                if (audioContent) {
                    if (!audioChannel) {
                        audioChannel = std::make_unique<OutgoingAudioChannel>(
                            call.get(),
                            channelManager.get(),
                            dtlsSrtpTransport.get(),
                            *audioContent,
                            workerThread(),
                            networkThread(),
                            &audioSink
                        );
                    }
                }
            }
        }
        if (videoChannelId) {
            if (const auto videoSsrc = contentNegotiationContext->outgoingChannelSsrc(*videoChannelId)) {
                if (videoChannel && videoChannel->ssrc() != videoSsrc.value()) {
                    videoChannel = nullptr;
                }
                std::optional<MediaContent> videoContent;
                for (const auto &content : coordinatedState->outgoingContents) {
                    if (content.type == MediaContent::Type::Video && content.ssrc == videoSsrc.value()) {
                        videoContent = content;
                        break;
                    }
                }
                if (videoContent) {
                    if (!videoChannel) {
                        videoChannel = std::make_unique<OutgoingVideoChannel>(
                            call.get(),
                            channelManager.get(),
                            dtlsSrtpTransport.get(),
                            *videoContent,
                            workerThread(),
                            networkThread(),
                            &videoSink
                        );
                    }
                }
            }
        }

        pendingContent.clear();
        for (const auto &content : coordinatedState->incomingContents) {
            switch (content.type) {
            case MediaContent::Type::Audio:
                incomingAudioChannels.clear();
                addIncomingSmartSource(std::to_string(content.ssrc), content);
                break;
            case MediaContent::Type::Video:
                incomingVideoChannels.clear();
                addIncomingSmartSource(std::to_string(content.ssrc), content);
                break;
            default:
                RTC_LOG(LS_WARNING) << "NativeNetworkingImpl unsupported incoming content type";
            }
        }
    }

    void NativeConnection::notifyStateUpdated() {
        ConnectionState newValue;
        if (failed) {
            newValue = ConnectionState::Failed;
        } else if (connected) {
            newValue = ConnectionState::Connected;
        } else {
            newValue = ConnectionState::Connecting;
        }
        std::weak_ptr weak(shared_from_this());
        signalingThread()->PostTask([weak, newValue] {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            (void) strong->connectionChangeCallback(newValue, strong->alreadyConnected);
            if (newValue == ConnectionState::Connected && !strong->alreadyConnected) {
                strong->alreadyConnected = true;
            }
        });
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void NativeConnection::candidateGathered(cricket::IceTransportInternal*, const cricket::Candidate& candidate) {
        assert(networkThread()->IsCurrent());
        std::weak_ptr weak(shared_from_this());
        signalingThread()->PostTask([weak, candidate] {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            cricket::Candidate patchedCandidate = candidate;
            patchedCandidate.set_component(1);
            webrtc::JsepIceCandidate iceCandidate{std::string(),0, patchedCandidate};
            (void) strong->iceCandidateCallback(IceCandidate(&iceCandidate));
        });
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    void NativeConnection::transportRouteChanged(std::optional<rtc::NetworkRoute> route) {
        assert(networkThread()->IsCurrent());
        if (route.has_value()) {
            RTC_LOG(LS_INFO) << "NativeNetworkingImpl route changed: " << route->DebugString();
            const bool localIsWifi = route->local.adapter_type() == rtc::AdapterType::ADAPTER_TYPE_WIFI;
            const bool remoteIsWifi = route->remote.adapter_type() == rtc::AdapterType::ADAPTER_TYPE_WIFI;
            RTC_LOG(LS_INFO) << "NativeNetworkingImpl is wifi: local=" << localIsWifi << ", remote=" << remoteIsWifi;
            const std::string localDescription = route->local.uses_turn() ? "turn" : "p2p";
            const std::string remoteDescription = route->remote.uses_turn() ? "turn" : "p2p";
            if (RouteDescription routeDescription(localDescription, remoteDescription); !currentRouteDescription || routeDescription != currentRouteDescription.value()) {
                currentRouteDescription = std::move(routeDescription);
                notifyStateUpdated();
            }
        }
    }

    void NativeConnection::candidatePairChanged(cricket::CandidatePairChangeEvent const& event) {
        ConnectionDescription connectionDescription;

        connectionDescription.local = connectionDescriptionFromCandidate(event.selected_candidate_pair.local);
        connectionDescription.remote = connectionDescriptionFromCandidate(event.selected_candidate_pair.remote);

        if (!currentConnectionDescription || currentConnectionDescription.value() != connectionDescription) {
            currentConnectionDescription = std::move(connectionDescription);
            notifyStateUpdated();
        }
    }

    void NativeConnection::start() {
        transportChannel->MaybeStartGathering();
        dataChannelInterface = std::make_unique<SctpDataChannelProviderInterfaceImpl>(
            environment(),
            dtlsTransport.get(),
            isOutgoing,
            networkThread()
        );
        std::weak_ptr weak(shared_from_this());
        dataChannelInterface->onMessageReceived([weak](const bytes::binary &data) {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            (void) strong->dataChannelMessageCallback(data);
        });
        dataChannelInterface->onStateChanged([weak](const bool isOpen) {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            if (!strong->dataChannelOpen && isOpen) {
                strong->dataChannelOpen = true;
                (void) strong->dataChannelOpenedCallback();
            } else {
                strong->dataChannelOpen = false;
            }
        });
        lastDisconnectedTimestamp = rtc::TimeMillis();
        checkConnectionTimeout();
    }

    void NativeConnection::close() {
        contentNegotiationContext = nullptr;
        NativeNetworkInterface::close();
    }

    void NativeConnection::addIceCandidate(const IceCandidate& rawCandidate) const {
        const bool standaloneReflectorMode = getCustomParameterBool("network_standalone_reflectors");
        const auto candidate = parseIceCandidate(rawCandidate)->candidate();
        if (standaloneReflectorMode) {
            if (absl::EndsWith(candidate.address().hostname(), ".reflector")) {
                return;
            }
        }
        std::weak_ptr weak(shared_from_this());
        networkThread()->PostTask([weak, candidate] {
            const auto strong = std::static_pointer_cast<const NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->transportChannel->AddRemoteCandidate(candidate);
        });
    }

    void NativeConnection::setRemoteParams(PeerIceParameters remoteIceParameters, std::unique_ptr<rtc::SSLFingerprint> fingerprint, const std::string& sslSetup) {
        std::weak_ptr weak(shared_from_this());
        networkThread()->PostTask([weak, remoteIceParameters = std::move(remoteIceParameters), fingerprint = std::move(fingerprint), sslSetup] {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->remoteParameters = remoteIceParameters;
            const cricket::IceParameters parameters(
                remoteIceParameters.ufrag,
                remoteIceParameters.pwd,
                remoteIceParameters.supportsRenomination
            );
            strong->transportChannel->SetRemoteIceParameters(parameters);
            rtc::SSLRole sslRole;
            if (sslSetup == "active") {
                sslRole = rtc::SSLRole::SSL_SERVER;
            } else if (sslSetup == "passive") {
                sslRole = rtc::SSLRole::SSL_CLIENT;
            } else {
                sslRole = strong->isOutgoing ? rtc::SSLRole::SSL_CLIENT : rtc::SSLRole::SSL_SERVER;
            }
            if (fingerprint) {
                strong->dtlsTransport->SetRemoteParameters(fingerprint->algorithm, fingerprint->digest.data(), fingerprint->digest.size(), sslRole);
            }
        });
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> NativeConnection::getPendingOffer() const {
        return contentNegotiationContext->getPendingOffer();
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> NativeConnection::setPendingAnswer(std::unique_ptr<ContentNegotiationContext::NegotiationContents> answer) const {
        return contentNegotiationContext->setPendingAnswer(std::move(answer));
    }

    std::unique_ptr<MediaTrackInterface> NativeConnection::addOutgoingTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) {
        if (const auto audioTrack = dynamic_cast<webrtc::AudioTrackInterface*>(track.get())) {
            audioChannelId = contentNegotiationContext->addOutgoingChannel(audioTrack);
        }
        if (const auto videoTrack = dynamic_cast<webrtc::VideoTrackInterface*>(track.get())) {
            videoChannelId = contentNegotiationContext->addOutgoingChannel(videoTrack);
        }
        return NativeNetworkInterface::addOutgoingTrack(track);
    }

    bool NativeConnection::isGroupConnection() const {
        return false;
    }

    void NativeConnection::checkConnectionTimeout() {
        std::weak_ptr weak(shared_from_this());
        networkThread()->PostDelayedTask([weak] {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            const int64_t currentTimestamp = rtc::TimeMillis();
            if (constexpr int64_t maxTimeout = 20000; !strong->connected && strong->lastDisconnectedTimestamp + maxTimeout < currentTimestamp) {
                RTC_LOG(LS_INFO) << "NativeNetworkingImpl timeout " << currentTimestamp - strong->lastDisconnectedTimestamp << " ms";
                strong->failed = true;
                strong->notifyStateUpdated();
                return;
            }
            strong->checkConnectionTimeout();
        }, webrtc::TimeDelta::Millis(1000));
    }
} // wrtc