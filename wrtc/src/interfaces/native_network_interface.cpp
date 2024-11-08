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
        availableVideoFormats = filterSupportedVideoFormats(factory->getSupportedVideoFormats());
    }

    void NativeNetworkInterface::addIncomingSmartSource(const std::string& endpoint, const MediaContent& mediaContent, const bool force) {
        if (pendingContent.contains(endpoint) && !force) {
            return;
        }
        bool isAddable = false;
        switch (mediaContent.type) {
        case MediaContent::Type::Audio:
            isAddable = audioIncoming;
            break;
        case MediaContent::Type::Video:
            if (mediaContent.isScreenCast()) {
                isAddable = screenIncoming;
            } else {
                isAddable = cameraIncoming;
            }
            break;
        }
        if (isAddable && mediaContent.type == MediaContent::Type::Audio) {
            if (incomingAudioChannels.size() > 10) {
                int64_t minActivity = INT64_MAX;
                const auto timestamp = rtc::TimeMillis();
                std::string minActivityChannelId;
                for (const auto& [channelId, channel] : incomingAudioChannels) {
                    if (const auto activity = channel->getActivity(); activity < minActivity && activity < timestamp - 1000) {
                        minActivity = activity;
                        minActivityChannelId = channelId;
                    }
                }
                if (!minActivityChannelId.empty()) {
                    removeIncomingAudio(minActivityChannelId);
                }
                if (incomingAudioChannels.size() > 10) {
                    RTC_LOG(LS_WARNING) << "Too many incoming audio channels, unable to add " << endpoint << " ssrc";
                    return;
                }
            }
            RTC_LOG(LS_INFO) << "Adding incoming audio channel with ssrc " << mediaContent.mainSsrc();
            if (const auto sink = remoteAudioSink.lock()) sink->addSource();
            incomingAudioChannels[endpoint] = std::make_unique<IncomingAudioChannel>(
                call.get(),
                channelManager.get(),
                dtlsSrtpTransport.get(),
                mediaContent,
                workerThread(),
                networkThread(),
                remoteAudioSink
            );
        } else if (isAddable && mediaContent.type == MediaContent::Type::Video) {
            RTC_LOG(LS_INFO) << "Adding incoming video channel with ssrc " << mediaContent.mainSsrc();
            incomingVideoChannels[endpoint] = std::make_unique<IncomingVideoChannel>(
                call.get(),
                channelManager.get(),
                dtlsSrtpTransport.get(),
                mediaContent.ssrcGroups,
                factory->ssrcGenerator(),
                availableVideoFormats,
                workerThread(),
                networkThread(),
                mediaContent.isScreenCast() ? remoteScreenCastSink : remoteVideoSink
            );
        }
        if (pendingContent.contains(endpoint)) {
            return;
        }
        int audioChannelsCount = 0;
        // ReSharper disable once CppUseElementsView
        for (const auto& [endpoint, content] : pendingContent) {
            if (content.type == MediaContent::Type::Audio) {
                audioChannelsCount++;
            }
        }
        if (audioChannelsCount >= 10) {
            return;
        }
        pendingContent[endpoint] = mediaContent;
    }

    void NativeNetworkInterface::removeIncomingAudio(const std::string& endpoint) {
        if (!pendingContent.contains(endpoint)) {
            return;
        }
        RTC_LOG(LS_INFO) << "Removing incoming audio channel with ssrc " << endpoint;
        if (incomingAudioChannels.contains(endpoint)) incomingAudioChannels.erase(endpoint);
        pendingContent.erase(endpoint);
        if (const auto sink = remoteAudioSink.lock()) sink->removeSource();
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

    std::vector<std::string> NativeNetworkInterface::getEndpoints() const {
        std::vector<std::string> endpoints;
        endpoints.reserve(incomingVideoChannels.size());
        // ReSharper disable once CppUseElementsView
        for (const auto &[endpoint, _] : incomingVideoChannels) {
            endpoints.push_back(endpoint);
        }
        return endpoints;
    }

    void NativeNetworkInterface::enableAudioIncoming(const bool enable) {
        if (audioIncoming == enable) {
            return;
        }
        NetworkInterface::enableAudioIncoming(enable);
        workerThread()->BlockingCall([&] {
            if (enable) {
                for (const auto& [endpoint, mediaContent] : pendingContent) {
                    if (mediaContent.type == MediaContent::Type::Audio) {
                        addIncomingSmartSource(endpoint, mediaContent, true);
                    }
                }
            } else {
                incomingAudioChannels.clear();
            }
        });
    }

    void NativeNetworkInterface::enableVideoIncoming(const bool enable, const bool isScreenCast) {
        if (isScreenCast) {
            if (cameraIncoming == enable) {
                return;
            }
        } else {
            if (screenIncoming == enable) {
                return;
            }
        }
        NetworkInterface::enableVideoIncoming(enable, isScreenCast);
        workerThread()->BlockingCall([&] {
            if (enable) {
                for (const auto& [endpoint, mediaContent] : pendingContent) {
                    if (mediaContent.type == MediaContent::Type::Video && mediaContent.isScreenCast() == isScreenCast) {
                        addIncomingSmartSource(endpoint, mediaContent, true);
                    }
                }
            } else {
                for (const auto& [endpoint, mediaContent] : pendingContent) {
                    if (mediaContent.type == MediaContent::Type::Video && mediaContent.isScreenCast() == isScreenCast) {
                        incomingVideoChannels.erase(endpoint);
                    }
                }
            }
        });
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
            signalingThread()->BlockingCall([] {});
        }
        NetworkInterface::close();
    }

    void NativeNetworkInterface::addIncomingAudioTrack(const std::weak_ptr<RemoteAudioSink>& sink) {
        remoteAudioSink = sink;
    }

    void NativeNetworkInterface::addIncomingVideoTrack(const std::weak_ptr<RemoteVideoSink>& sink, const bool isScreenCast) {
        if (isScreenCast) {
            remoteScreenCastSink = sink;
        } else {
            remoteVideoSink = sink;
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

    std::vector<webrtc::SdpVideoFormat> NativeNetworkInterface::filterSupportedVideoFormats(std::vector<webrtc::SdpVideoFormat> const& formats) {
        std::vector<webrtc::SdpVideoFormat> filteredFormats;

        std::vector<std::string> filterCodecNames = {
            cricket::kVp8CodecName,
            cricket::kVp9CodecName,
            cricket::kH264CodecName
        };

        std::vector<webrtc::SdpVideoFormat> vp9Formats;
        std::vector<webrtc::SdpVideoFormat> h264Formats;

        for (const auto &format : formats) {
            if (std::ranges::find(filterCodecNames, format.name) == filterCodecNames.end()) {
                continue;
            }

            if (format.name == cricket::kVp9CodecName) {
                vp9Formats.push_back(format);
            } else if (format.name == cricket::kH264CodecName) {
                h264Formats.push_back(format);
            } else {
                filteredFormats.push_back(format);
            }
        }

        if (!vp9Formats.empty()) {
            bool added = false;
            for (const auto &format : vp9Formats) {
                if (added) {
                    break;
                }
                for (const auto & [fst, snd] : format.parameters) {
                    if (fst == "profile-id") {
                        if (snd == "0") {
                            filteredFormats.push_back(format);
                            added = true;
                            break;
                        }
                    }
                }
            }

            if (!added) {
                filteredFormats.push_back(vp9Formats[0]);
            }
        }

        if (!h264Formats.empty()) {
            std::ranges::sort(h264Formats, [](const webrtc::SdpVideoFormat &lhs, const webrtc::SdpVideoFormat &rhs) {
                auto [lProfileLevelId, lPacketizationMode, lLevelAssymetryAllowed] = parseH264FormatParameters(lhs);
                auto [rProfileLevelId, rPacketizationMode, rLevelAssymetryAllowed] = parseH264FormatParameters(rhs);

                const int lhsLevelIdPriority = getH264ProfileLevelIdPriority(lProfileLevelId);
                const int lhsPacketizationModePriority = getH264PacketizationModePriority(lPacketizationMode);
                const int lhsLevelAssymetryAllowedPriority = getH264LevelAssymetryAllowedPriority(lLevelAssymetryAllowed);

                const int rhsLevelIdPriority = getH264ProfileLevelIdPriority(rProfileLevelId);
                const int rhsPacketizationModePriority = getH264PacketizationModePriority(rPacketizationMode);
                const int rhsLevelAssymetryAllowedPriority = getH264LevelAssymetryAllowedPriority(rLevelAssymetryAllowed);

                if (lhsLevelIdPriority != rhsLevelIdPriority) {
                    return lhsLevelIdPriority < rhsLevelIdPriority;
                }
                if (lhsPacketizationModePriority != rhsPacketizationModePriority) {
                    return lhsPacketizationModePriority < rhsPacketizationModePriority;
                }
                if (lhsLevelAssymetryAllowedPriority != rhsLevelAssymetryAllowedPriority) {
                    return lhsLevelAssymetryAllowedPriority < rhsLevelAssymetryAllowedPriority;
                }

                return false;
            });

            filteredFormats.push_back(h264Formats[0]);
        }

        return filteredFormats;
    }

    NativeNetworkInterface::H264FormatParameters NativeNetworkInterface::parseH264FormatParameters(webrtc::SdpVideoFormat const& format) {
        H264FormatParameters result;
        for (const auto & [fst, snd] : format.parameters) {
            if (fst == "profile-level-id") {
                result.profileLevelId = snd;
            } else if (fst == "packetization-mode") {
                result.packetizationMode = snd;
            } else if (fst == "level-asymmetry-allowed") {
                result.levelAssymetryAllowed = snd;
            }
        }
        return result;
    }

    int NativeNetworkInterface::getH264ProfileLevelIdPriority(std::string const& profileLevelId) {
        if (profileLevelId == cricket::kH264ProfileLevelConstrainedHigh) {
            return 0;
        }
        if (profileLevelId == cricket::kH264ProfileLevelConstrainedBaseline) {
            return 1;
        }
        return 2;
    }

    int NativeNetworkInterface::getH264PacketizationModePriority(std::string const& packetizationMode) {
        if (packetizationMode == "1") {
            return 0;
        }
        return 1;
    }

    int NativeNetworkInterface::getH264LevelAssymetryAllowedPriority(std::string const& levelAssymetryAllowed) {
        if (levelAssymetryAllowed == "1") {
            return 0;
        }
        return 1;
    }
} // wrtc