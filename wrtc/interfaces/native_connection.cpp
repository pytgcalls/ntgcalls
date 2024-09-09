//
// Created by Laky64 on 29/03/2024.
//

#include "native_connection.hpp"

#include <memory>
#include <utility>
#include <api/enable_media.h>
#include <p2p/base/p2p_constants.h>
#include <rtc_base/crypto_random.h>
#include <rtc_base/rtc_certificate_generator.h>
#include <p2p/base/basic_async_resolver_factory.h>
#include <p2p/base/p2p_transport_channel.h>
#include <pc/media_factory.h>

#include "reflector_relay_port_factory.hpp"
#include "media/rtc_audio_source.hpp"
#include "wrtc/exceptions.hpp"


namespace wrtc {
    NativeConnection::NativeConnection(std::vector<RTCServer> rtcServers,
        const bool enableP2P,
        const bool isOutgoing):
    isOutgoing(isOutgoing),
    enableP2P(enableP2P),
    rtcServers(std::move(rtcServers)),
    eventLog(std::make_unique<webrtc::RtcEventLogNull>()) {
        networkThread()->PostTask([this] {
            localParameters = PeerIceParameters(
                rtc::CreateRandomString(cricket::ICE_UFRAG_LENGTH),
                rtc::CreateRandomString(cricket::ICE_PWD_LENGTH),
                true
            );
            localCertificate = rtc::RTCCertificateGenerator::GenerateCertificate(
                rtc::KeyParams(rtc::KT_ECDSA),
                absl::nullopt
            );
            asyncResolverFactory = std::make_unique<webrtc::BasicAsyncDnsResolverFactory>();
            dtlsSrtpTransport = std::make_unique<webrtc::DtlsSrtpTransport>(true, factory->fieldTrials());
            dtlsSrtpTransport->SetDtlsTransports(nullptr, nullptr);
            dtlsSrtpTransport->SetActiveResetSrtpParams(false);
            dtlsSrtpTransport->SubscribeReadyToSend(this, [this](const bool readyToSend) {
                DtlsReadyToSend(readyToSend);
            });
            dtlsSrtpTransport->SubscribeRtcpPacketReceived(this, [this](const rtc::CopyOnWriteBuffer* packet, int64_t) {
                workerThread()->PostTask([this, packet = *packet] {
                    call->Receiver()->DeliverRtcpPacket(packet);
                });
            });
            resetDtlsSrtpTransport();
        });
        channelManager = std::make_unique<ChannelManager>(
            factory->mediaEngine(),
            workerThread(),
            networkThread(),
            signalingThread()
        );
        workerThread()->BlockingCall([&] {
            webrtc::CallConfig callConfig(factory->environment(), networkThread());
            callConfig.audio_state = factory->mediaEngine()->voice().GetAudioState();
            call = factory->mediaFactory()->CreateCall(callConfig);
        });
        contentNegotiationContext = std::make_unique<ContentNegotiationContext>(factory->fieldTrials(), isOutgoing, factory->mediaEngine(), factory->ssrcGenerator());
        contentNegotiationContext->copyCodecsFromChannelManager(factory->mediaEngine(), false);
        networkThread()->PostTask([this] {
            start();
        });
    }

    void NativeConnection::resetDtlsSrtpTransport() {
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
        portAllocator = std::make_unique<cricket::BasicPortAllocator>(
            factory->networkManager(),
            factory->socketFactory(),
            nullptr,
            relayPortFactory.get()
        );
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
        portAllocator->Initialize();
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
        portAllocator->SetConfiguration(stunServers, turnServers, 0, webrtc::NO_PRUNE);
        webrtc::IceTransportInit iceTransportInit;
        iceTransportInit.set_port_allocator(portAllocator.get());
        iceTransportInit.set_async_dns_resolver_factory(asyncResolverFactory.get());
        transportChannel = cricket::P2PTransportChannel::Create("transport", 0, std::move(iceTransportInit));
        cricket::IceConfig iceConfig;
        iceConfig.continual_gathering_policy = cricket::GATHER_CONTINUALLY;
        iceConfig.prioritize_most_likely_candidate_pairs = true;
        iceConfig.regather_on_failed_networks_interval = 8000;
        if (getCustomParameterBool("network_skip_initial_ping")) {
            iceConfig.presume_writable_when_fully_relayed = true;
        }
        transportChannel->SetIceConfig(iceConfig);
        const cricket::IceParameters localIceParameters(
            localParameters.ufrag,
            localParameters.pwd,
            localParameters.supportsRenomination
        );
        transportChannel->SetIceParameters(localIceParameters);
        transportChannel->SetIceRole(isOutgoing ? cricket::ICEROLE_CONTROLLING : cricket::ICEROLE_CONTROLLED);
        transportChannel->SetRemoteIceMode(cricket::ICEMODE_FULL);
        transportChannel->SignalCandidateGathered.connect(this, &NativeConnection::candidateGathered);
        transportChannel->SignalIceTransportStateChanged.connect(this, &NativeConnection::transportStateChanged);
        transportChannel->SetCandidatePairChangeCallback([this](cricket::CandidatePairChangeEvent const &event) {
            candidatePairChanged(event);
        });
        transportChannel->SignalNetworkRouteChanged.connect(this, &NativeConnection::transportRouteChanged);
        dtlsTransport = std::make_unique<cricket::DtlsTransport>(transportChannel.get(), getDefaultCryptoOptions(), nullptr);
        dtlsTransport->SignalWritableState.connect(this, &NativeConnection::OnTransportWritableState_n);
        dtlsTransport->SignalReceivingState.connect(this, &NativeConnection::OnTransportReceivingState_n);
        dtlsTransport->SetLocalCertificate(localCertificate);
        dtlsSrtpTransport->SetDtlsTransports(dtlsTransport.get(), nullptr);
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
    }

    NativeConnection::~NativeConnection() {
        close();
    }

    void NativeConnection::notifyStateUpdated() const {
        ConnectionState newValue;
        if (failed) {
            newValue = ConnectionState::Failed;
        } else if (connected) {
            newValue = ConnectionState::Connected;
        } else {
            newValue = ConnectionState::Connecting;
        }
        signalingThread()->PostTask([this, newValue] {
            (void) connectionChangeCallback(newValue);
        });
    }

    void NativeConnection::DtlsReadyToSend(const bool isReadyToSend) {
        UpdateAggregateStates_n();

        if (isReadyToSend) {
            networkThread()->PostTask([this] {
                UpdateAggregateStates_n();
            });
        }
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void NativeConnection::candidateGathered(cricket::IceTransportInternal*, const cricket::Candidate& candidate) {
        assert(networkThread()->IsCurrent());
        signalingThread()->PostTask([this, candidate] {
            cricket::Candidate patchedCandidate = candidate;
            patchedCandidate.set_component(1);
            webrtc::JsepIceCandidate iceCandidate{std::string(),0, patchedCandidate};
            (void) iceCandidateCallback(IceCandidate(&iceCandidate));
        });
    }

    void NativeConnection::transportStateChanged(cricket::IceTransportInternal*) {
        UpdateAggregateStates_n();
    }

    // ReSharper disable once CppPassValueParameterByConstReference
    void NativeConnection::transportRouteChanged(absl::optional<rtc::NetworkRoute> route) {
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

    void NativeConnection::OnTransportWritableState_n(rtc::PacketTransportInternal*) {
        assert(networkThread()->IsCurrent());
        UpdateAggregateStates_n();
    }

    void NativeConnection::OnTransportReceivingState_n(rtc::PacketTransportInternal*){
        assert(networkThread()->IsCurrent());
        UpdateAggregateStates_n();
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

    void NativeConnection::UpdateAggregateStates_n() {
        assert(networkThread()->IsCurrent());
        const auto state = transportChannel->GetIceTransportState();
        bool isConnected = false;
        switch (state) {
            case webrtc::IceTransportState::kConnected:
            case webrtc::IceTransportState::kCompleted:
                isConnected = true;
                break;
            default:
                break;
        }
        if (!dtlsSrtpTransport->IsWritable(false)) {
            isConnected = false;
        }
        if (connected != isConnected) {
            connected = isConnected;
            if (!isConnected) {
                lastDisconnectedTimestamp = rtc::TimeMillis();
            }
            notifyStateUpdated();
            if (dataChannelInterface) {
                dataChannelInterface->updateIsConnected(isConnected);
            }
        }
    }

    webrtc::CryptoOptions NativeConnection::getDefaultCryptoOptions() {
        auto options = webrtc::CryptoOptions();
        options.srtp.enable_aes128_sha1_80_crypto_cipher = true;
        options.srtp.enable_gcm_crypto_suites = true;
        return options;
    }

    void NativeConnection::start() {
        transportChannel->MaybeStartGathering();
        dataChannelInterface = std::make_unique<SctpDataChannelProviderInterfaceImpl>(
            environment(),
            dtlsTransport.get(),
            isOutgoing,
            networkThread(),
            signalingThread()
        );
        dataChannelInterface->onStateChanged([this](const bool isOpen) {
            if (!dataChannelOpen && isOpen) {
                dataChannelOpen = true;
                (void) dataChannelOpenedCallback();
            } else {
                dataChannelOpen = false;
            }
        });
        lastDisconnectedTimestamp = rtc::TimeMillis();
        checkConnectionTimeout();
    }

    void NativeConnection::close() {
        isExiting = true;
        audioChannel = nullptr;
        videoChannel = nullptr;
        channelManager = nullptr;
        if (factory) {
            workerThread()->BlockingCall([&] {
                call = nullptr;
            });
        }
        contentNegotiationContext = nullptr;
        if (factory) {
            networkThread()->BlockingCall([&] {
                if (transportChannel) {
                    transportChannel->SignalCandidateGathered.disconnect(this);
                    transportChannel->SignalIceTransportStateChanged.disconnect(this);
                    transportChannel->SignalNetworkRouteChanged.disconnect(this);
                }
                dataChannelInterface = nullptr;
                if (dtlsTransport) {
                    dtlsTransport->SignalWritableState.disconnect(this);
                    dtlsTransport->SignalReceivingState.disconnect(this);
                }
                if (dtlsSrtpTransport) {
                    dtlsSrtpTransport->SetDtlsTransports(nullptr, nullptr);
                }
                dtlsTransport = nullptr;
                transportChannel = nullptr;
                portAllocator = nullptr;
            });
        }
        NetworkInterface::close();
    }

    void NativeConnection::sendDataChannelMessage(const bytes::binary& data) const {
        networkThread()->PostTask([this, data] {
            if (dataChannelInterface) {
                dataChannelInterface->sendDataChannelMessage(data);
            }
        });
    }

    void NativeConnection::addIceCandidate(const IceCandidate& rawCandidate) const {
        const bool standaloneReflectorMode = getCustomParameterBool("network_standalone_reflectors");
        const auto candidate = parseIceCandidate(rawCandidate)->candidate();
        if (standaloneReflectorMode) {
            if (absl::EndsWith(candidate.address().hostname(), ".reflector")) {
                return;
            }
        }
        networkThread()->PostTask([this, candidate] {
            transportChannel->AddRemoteCandidate(candidate);
        });
    }

    void NativeConnection::setRemoteParams(PeerIceParameters remoteIceParameters, std::unique_ptr<rtc::SSLFingerprint> fingerprint, const std::string& sslSetup) {
        networkThread()->PostTask([this, remoteIceParameters = std::move(remoteIceParameters), fingerprint = std::move(fingerprint), sslSetup] {
            remoteParameters = remoteIceParameters;
            const cricket::IceParameters parameters(
                remoteIceParameters.ufrag,
                remoteIceParameters.pwd,
                remoteIceParameters.supportsRenomination
            );
            transportChannel->SetRemoteIceParameters(parameters);
            rtc::SSLRole sslRole;
            if (sslSetup == "active") {
                sslRole = rtc::SSLRole::SSL_SERVER;
            } else if (sslSetup == "passive") {
                sslRole = rtc::SSLRole::SSL_CLIENT;
            } else {
                sslRole = isOutgoing ? rtc::SSLRole::SSL_CLIENT : rtc::SSLRole::SSL_SERVER;
            }
            if (fingerprint) {
                dtlsTransport->SetRemoteParameters(fingerprint->algorithm, fingerprint->digest.data(), fingerprint->digest.size(), sslRole);
            }
        });
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> NativeConnection::getPendingOffer() const {
        return contentNegotiationContext->getPendingOffer();
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> NativeConnection::setPendingAnswer(std::unique_ptr<ContentNegotiationContext::NegotiationContents> answer) const {
        return contentNegotiationContext->setPendingAnswer(std::move(answer));
    }

    std::unique_ptr<MediaTrackInterface> NativeConnection::addTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) {
        if (const auto audioTrack = dynamic_cast<webrtc::AudioTrackInterface*>(track.get())) {
            audioChannelId = contentNegotiationContext->addOutgoingChannel(audioTrack);
            audioTrack->AddSink(&audioSink);
            return std::make_unique<MediaTrackInterface>([this](const bool enable) {
                if (audioChannel != nullptr) {
                    audioChannel->set_enabled(enable);
                }
            });
        }
        if (const auto videoTrack = dynamic_cast<webrtc::VideoTrackInterface*>(track.get())) {
            videoChannelId = contentNegotiationContext->addOutgoingChannel(videoTrack);
            videoTrack->AddOrUpdateSink(&videoSink, rtc::VideoSinkWants());
            return std::make_unique<MediaTrackInterface>([this](const bool enable) {
                if (videoChannel != nullptr) {
                    videoChannel->set_enabled(enable);
                }
            });
        }
        throw RTCException("Unsupported track type");
    }

    std::unique_ptr<rtc::SSLFingerprint> NativeConnection::localFingerprint() const {
        const auto certificate = localCertificate;
        if (!certificate) {
            return nullptr;
        }
        return rtc::SSLFingerprint::CreateFromCertificate(*certificate);
    }

    PeerIceParameters NativeConnection::localIceParameters() {
        return localParameters;
    }

    void NativeConnection::checkConnectionTimeout() {
        networkThread()->PostDelayedTask([this] {
            if (isExiting) return;
            const int64_t currentTimestamp = rtc::TimeMillis();
            if (constexpr int64_t maxTimeout = 20000; !connected && lastDisconnectedTimestamp + maxTimeout < currentTimestamp) {
                RTC_LOG(LS_INFO) << "NativeNetworkingImpl timeout " << currentTimestamp - lastDisconnectedTimestamp << " ms";
                failed = true;
                notifyStateUpdated();
                return;
            }
            checkConnectionTimeout();
        }, webrtc::TimeDelta::Millis(1000));
    }
} // wrtc