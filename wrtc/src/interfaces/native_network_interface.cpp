//
// Created by Laky64 on 01/10/24.
//

#include <p2p/base/basic_async_resolver_factory.h>
#include <p2p/base/p2p_constants.h>
#include <p2p/client/basic_port_allocator.h>
#include <pc/media_factory.h>
#include <rtc_base/crypto_random.h>
#include <rtc_base/rtc_certificate_generator.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/wrapped_dtls_srtp_transport.hpp>

namespace wrtc {
    void NativeNetworkInterface::initConnection(bool supportsPacketSending) {
        networkThread()->PostTask([this, supportsPacketSending] {
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
            dtlsSrtpTransport = std::make_unique<WrappedDtlsSrtpTransport>(
                true,
                factory->fieldTrials(),
                [this](const webrtc::RtpPacketReceived& packet) {
                    workerThread()->PostTask([this, packet] {
                        RtpPacketReceived(packet);
                    });
                }
            );
            dtlsSrtpTransport->SetDtlsTransports(nullptr, nullptr);
            dtlsSrtpTransport->SetActiveResetSrtpParams(false);
            dtlsSrtpTransport->SubscribeReadyToSend(this, [this](const bool readyToSend) {
                DtlsReadyToSend(readyToSend);
            });
            dtlsSrtpTransport->SubscribeRtcpPacketReceived(this, [this](const rtc::CopyOnWriteBuffer* packet, int64_t) {
               workerThread()->PostTask([this, packet = *packet] {
                   if (call) call->Receiver()->DeliverRtcpPacket(packet);
               });
            });
            if (supportsPacketSending) {
                dtlsSrtpTransport->SubscribeSentPacket(this, [this](const rtc::SentPacket& packet) {
                    workerThread()->PostTask([this, packet] {
                        if (call) call->OnSentPacket(packet);
                    });
                });
            }
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
            call = factory->mediaFactory()->CreateCall(std::move(callConfig));
        });
    }

    void NativeNetworkInterface::DtlsReadyToSend(const bool isReadyToSend) {
        UpdateAggregateStates_n();

        if (isReadyToSend) {
            networkThread()->PostTask([this] {
                UpdateAggregateStates_n();
            });
        }
    }

    void NativeNetworkInterface::UpdateAggregateStates_n() {
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
            stateUpdated(isConnected);
            if (dataChannelInterface) {
                dataChannelInterface->updateIsConnected(isConnected);
            }
        }
    }

    void NativeNetworkInterface::resetDtlsSrtpTransport() {
        portAllocator = std::make_unique<cricket::BasicPortAllocator>(
            factory->networkManager(),
            factory->socketFactory(),
            nullptr,
            getRelayPortFactory()
        );
        setPortAllocatorFlags(portAllocator.get());
        portAllocator->Initialize();
        auto [stunServers, turnServers] = getStunAndTurnServers();
        portAllocator->SetConfiguration(stunServers, turnServers, candidatePoolSize(), webrtc::NO_PRUNE);

        webrtc::IceTransportInit iceTransportInit;
        iceTransportInit.set_port_allocator(portAllocator.get());
        iceTransportInit.set_async_dns_resolver_factory(asyncResolverFactory.get());
        transportChannel = cricket::P2PTransportChannel::Create("transport", 0, std::move(iceTransportInit));

        cricket::IceConfig iceConfig;
        iceConfig.continual_gathering_policy = cricket::GATHER_CONTINUALLY;
        iceConfig.prioritize_most_likely_candidate_pairs = true;
        iceConfig.regather_on_failed_networks_interval = getRegatherOnFailedNetworksInterval();
        if (getCustomParameterBool("network_skip_initial_ping")) {
            iceConfig.presume_writable_when_fully_relayed = true;
        }
        transportChannel->SetIceConfig(iceConfig);

        const cricket::IceParameters localIceParameters(
            localParameters.ufrag,
            localParameters.pwd,
            supportsRenomination()
        );
        transportChannel->SetIceParameters(localIceParameters);
        transportChannel->SetIceRole(iceRole());
        transportChannel->SignalIceTransportStateChanged.connect(this, &NativeNetworkInterface::transportStateChanged);
        registerTransportCallbacks(transportChannel.get());

        dtlsTransport = std::make_unique<cricket::DtlsTransport>(transportChannel.get(), getDefaultCryptoOptions(), nullptr);
        dtlsTransport->SignalWritableState.connect(this, &NativeNetworkInterface::OnTransportWritableState_n);
        dtlsTransport->SignalReceivingState.connect(this, &NativeNetworkInterface::OnTransportReceivingState_n);
        if (const auto role = dtlsRole(); role.has_value()) {
            dtlsTransport->SetDtlsRole(role.value());
        }
        dtlsTransport->SetLocalCertificate(localCertificate);
        dtlsSrtpTransport->SetDtlsTransports(dtlsTransport.get(), nullptr);
    }


    void NativeNetworkInterface::transportStateChanged(cricket::IceTransportInternal*) {
        UpdateAggregateStates_n();
    }

    webrtc::CryptoOptions NativeNetworkInterface::getDefaultCryptoOptions() {
        auto options = webrtc::CryptoOptions();
        options.srtp.enable_aes128_sha1_80_crypto_cipher = true;
        options.srtp.enable_gcm_crypto_suites = true;
        return options;
    }

    void NativeNetworkInterface::OnTransportWritableState_n(rtc::PacketTransportInternal*) {
        assert(networkThread()->IsCurrent());
        UpdateAggregateStates_n();
    }

    void NativeNetworkInterface::OnTransportReceivingState_n(rtc::PacketTransportInternal*){
        assert(networkThread()->IsCurrent());
        UpdateAggregateStates_n();
    }

    std::unique_ptr<rtc::SSLFingerprint> NativeNetworkInterface::localFingerprint() const {
        const auto certificate = localCertificate;
        if (!certificate) {
            return nullptr;
        }
        return rtc::SSLFingerprint::CreateFromCertificate(*certificate);
    }

    void NativeNetworkInterface::close() {
        workerThread()->BlockingCall([&] {
            audioChannel = nullptr;
            videoChannel = nullptr;
            incomingAudioChannels.clear();
            incomingVideoChannels.clear();
        });
        channelManager = nullptr;
        if (factory) {
            workerThread()->BlockingCall([&] {
                call = nullptr;
            });
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
                    dtlsSrtpTransport->UnsubscribeSentPacket(this);
                    dtlsSrtpTransport->SetDtlsTransports(nullptr, nullptr);
                }
                dtlsTransport = nullptr;
                transportChannel = nullptr;
                portAllocator = nullptr;
            });
        }
        NetworkInterface::close();
    }

    void NativeNetworkInterface::addIncomingTrack(const std::weak_ptr<RemoteMediaInterface> remoteSink) {
        if (const auto audioSink = std::dynamic_pointer_cast<RemoteAudioSink>(remoteSink.lock())) {
            remoteAudioSink = audioSink;
        } else if (const auto videoSink = std::dynamic_pointer_cast<RemoteVideoSink>(remoteSink.lock())) {
            remoteVideoSink = videoSink;
        }
    }

    std::unique_ptr<MediaTrackInterface> NativeNetworkInterface::addOutgoingTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) {
        if (const auto audioTrack = dynamic_cast<webrtc::AudioTrackInterface*>(track.get())) {
            audioTrack->AddSink(&audioSink);
            return std::make_unique<MediaTrackInterface>([this](const bool enable) {
                if (audioChannel != nullptr) {
                    audioChannel->set_enabled(enable);
                }
            });
        }
        if (const auto videoTrack = dynamic_cast<webrtc::VideoTrackInterface*>(track.get())) {
            videoTrack->AddOrUpdateSink(&videoSink, rtc::VideoSinkWants());
            return std::make_unique<MediaTrackInterface>([this](const bool enable) {
                if (videoChannel != nullptr) {
                    videoChannel->set_enabled(enable);
                }
            });
        }
        throw RTCException("Unsupported track type");
    }

    PeerIceParameters NativeNetworkInterface::localIceParameters() {
        return localParameters;
    }

    void NativeNetworkInterface::sendDataChannelMessage(const bytes::binary& data) const {
        networkThread()->PostTask([this, data] {
            if (dataChannelInterface) {
                dataChannelInterface->sendDataChannelMessage(data);
            }
        });
    }
} // wrtc