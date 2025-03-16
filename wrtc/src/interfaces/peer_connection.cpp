//
// Created by Laky64 on 16/08/2023.
//

#include <wrtc/interfaces/peer_connection.hpp>

#include <future>
#include <wrtc/exceptions.hpp>

#include <wrtc/interfaces/peer_connection/set_session_description_observer.hpp>

namespace wrtc {

    PeerConnection::PeerConnection(const webrtc::PeerConnectionInterface::IceServers& servers, const bool allowAttachDataChannel, const bool allowP2P): allowAttachDataChannel(allowAttachDataChannel) {
        webrtc::PeerConnectionInterface::RTCConfiguration config;
        if (allowP2P) {
            config.type = webrtc::PeerConnectionInterface::IceTransportsType::kAll;
        } else {
            config.type = webrtc::PeerConnectionInterface::IceTransportsType::kRelay;
        }
        config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
        config.bundle_policy = webrtc::PeerConnectionInterface::kBundlePolicyMaxBundle;
        config.servers = servers;
        config.enable_ice_renomination = true;
        config.rtcp_mux_policy = webrtc::PeerConnectionInterface::RtcpMuxPolicy::kRtcpMuxPolicyRequire;
        config.enable_implicit_rollback = true;
        config.continual_gathering_policy = webrtc::PeerConnectionInterface::ContinualGatheringPolicy::GATHER_CONTINUALLY;
        config.audio_jitter_buffer_fast_accelerate = true;

        webrtc::PeerConnectionDependencies dependencies(this);

        auto result = factory->factory()->CreatePeerConnectionOrError(
            config, std::move(dependencies)
        );

        if (!result.ok()) {
            throw wrapRTCError(result.error());
        }

        peerConnection = result.MoveValue();
    }

    std::optional<Description> PeerConnection::localDescription() const {
        if (peerConnection) {
            if (const auto raw_description = peerConnection->local_description()) {
                std::string sdp;
                raw_description->ToString(&sdp);
                return Description(raw_description->GetType(), sdp);
            }
        }
        return std::nullopt;
    }

    void PeerConnection::setLocalDescription(const std::function<void()>& onSuccess, const std::function<void(const std::exception_ptr&)>& onError) const {
        if (!peerConnection || peerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setLocalDescription' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }
        const rtc::scoped_refptr<webrtc::SetLocalDescriptionObserverInterface> observer(new rtc::RefCountedObject<SetSessionDescriptionObserver>(
            onSuccess,
            onError
        ));
        peerConnection->SetLocalDescription(observer);
    }

    void PeerConnection::setLocalDescription() const {
        std::promise<void> promise;
        setLocalDescription(
            [&] {
                promise.set_value();
            },
            [&] (const std::exception_ptr& e) {
                promise.set_exception(e);
            }
        );
        if (promise.get_future().wait_for(std::chrono::seconds(60)) != std::future_status::ready) {
            throw RTCException("Failed to execute 'setRemoteDescription' on 'PeerConnection': The operation timed out.");
        }
    }

    void PeerConnection::setRemoteDescription(const Description& description, const std::function<void()>& onSuccess,const std::function<void(const std::exception_ptr&)>& onError) const {
        if (!peerConnection || peerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setRemoteDescription' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }
        webrtc::SdpParseError sdpParseError;
        auto remoteDescription = CreateSessionDescription(description.type(), description.sdp(), &sdpParseError);
        if (!remoteDescription) {
            throw wrapSdpParseError(sdpParseError);
        }
        const rtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface> observer(new rtc::RefCountedObject<SetSessionDescriptionObserver>(
            onSuccess,
            onError
        ));
        peerConnection->SetRemoteDescription(std::move(remoteDescription), observer);
    }

    void PeerConnection::setRemoteDescription(const Description& description) const {
        std::promise<void> promise;
        setRemoteDescription(
            description,
            [&] {
                promise.set_value();
            },
            [&] (const std::exception_ptr& e) {
                promise.set_exception(e);
            }
        );
        if (promise.get_future().wait_for(std::chrono::seconds(60)) != std::future_status::ready) {
            throw RTCException("Failed to execute 'setRemoteDescription' on 'PeerConnection': The operation timed out.");
        }
    }

    void PeerConnection::addIceCandidate(const IceCandidate& rawCandidate) const {
        peerConnection->AddIceCandidate(parseIceCandidate(rawCandidate));
    }

    std::unique_ptr<MediaTrackInterface> PeerConnection::addOutgoingTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) {
        if (!peerConnection) {
            throw RTCException("Cannot add track; PeerConnection is closed");
        }
        if (const auto result = peerConnection->AddTrack(track, {}); !result.ok()) {
            throw wrapRTCError(result.error());
        }
        return std::make_unique<MediaTrackInterface>([track](const bool enable) {
            if (track != nullptr) {
                track->set_enabled(enable);
            }
        });
    }

    void PeerConnection::restartIce() const {
        if (peerConnection) {
            peerConnection->RestartIce();
        }
    }

    void PeerConnection::createDataChannel(const std::string& label) {
        const webrtc::DataChannelInit dataChannelInit;
        if (webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::DataChannelInterface>> dataChannelOrError = peerConnection->CreateDataChannelOrError(label, &dataChannelInit); dataChannelOrError.ok()) {
            attachDataChannel(dataChannelOrError.value());
        } else {
            throw wrapRTCError(dataChannelOrError.error());
        }
    }

    void PeerConnection::sendDataChannelMessage(const bytes::binary& data) const {
        if (dataChannel) {
            const std::string stringData(data.begin(), data.end());
            dataChannel->Send(webrtc::DataBuffer(stringData));
        } else {
            throw RTCException("Cannot send data channel message; Data channel is not open");
        }
    }

    void PeerConnection::close() {
        if (dataChannel) {
            dataChannel->UnregisterObserver();
            dataChannel = nullptr;
        }
        if (dataChannelObserver) {
            dataChannelObserver = nullptr;
        }
        if (peerConnection) {
            peerConnection->Close();
            peerConnection = nullptr;
        }
        NetworkInterface::close();
    }

    SignalingState PeerConnection::signalingState() const {
        return parseSignalingState(peerConnection->signaling_state());
    }

    void PeerConnection::onIceStateChange(const std::function<void(IceState)> &callback) {
        stateChangeCallback = callback;
    }

    void PeerConnection::onGatheringStateChange(const std::function<void(GatheringState)> &callback) {
        gatheringStateChangeCallback = callback;
    }

    void PeerConnection::onSignalingStateChange(const std::function<void(SignalingState)> &callback) {
        signalingStateChangeCallback = callback;
    }

    void PeerConnection::onRenegotiationNeeded(const std::function<void()>& callback) {
        renegotiationNeededCallback = callback;
    }

    void PeerConnection::onDataChannelMessage(const std::function<void(bytes::binary)>& callback) {
        dataChannelMessageCallback = callback;
    }

    void PeerConnection::OnSignalingChange(const webrtc::PeerConnectionInterface::SignalingState new_state) {
        (void) signalingStateChangeCallback(parseSignalingState(new_state));
    }

    SignalingState PeerConnection::parseSignalingState(const webrtc::PeerConnectionInterface::SignalingState state) {
        auto newValue = SignalingState::Unknown;
        switch (state) {
        case webrtc::PeerConnectionInterface::kStable:
            newValue = SignalingState::Stable;
            break;
        case webrtc::PeerConnectionInterface::kHaveLocalOffer:
            newValue = SignalingState::HaveLocalOffer;
            break;
        case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
            newValue = SignalingState::HaveLocalPranswer;
            break;
        case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
            newValue = SignalingState::HaveRemoteOffer;
            break;
        case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
            newValue = SignalingState::HaveRemotePranswer;
            break;
        case webrtc::PeerConnectionInterface::kClosed:
            newValue = SignalingState::Closed;
            break;
        }
        return newValue;
    }

    void PeerConnection::onDataChannelStateUpdated() {
        if (dataChannel) {
            if (dataChannel->state() == webrtc::DataChannelInterface::DataState::kOpen) {
                if (!dataChannelOpen) {
                    dataChannelOpen = true;
                    (void) dataChannelOpenedCallback();
                }
            } else {
                dataChannelOpen = false;
            }
        }
    }

    void PeerConnection::attachDataChannel(const rtc::scoped_refptr<webrtc::DataChannelInterface>& dataChannel) {
        DataChannelObserverImpl::Parameters dataChannelObserverParams;
        dataChannelObserverParams.onStateChange = [this] {
            signalingThread() -> PostTask([this] {
                onDataChannelStateUpdated();
            });
        };
        dataChannelObserverParams.onMessage = [this](const webrtc::DataBuffer &buffer) {
            signalingThread() -> PostTask([this, buffer] {
                if (!buffer.binary) {
                    dataChannelMessageCallback(bytes::binary(buffer.data.data(), buffer.data.data() + buffer.data.size()));
                }
            });
        };
        this -> dataChannel = dataChannel;
        dataChannelObserver = std::make_unique<DataChannelObserverImpl>(std::move(dataChannelObserverParams));
        onDataChannelStateUpdated();
        dataChannel->RegisterObserver(dataChannelObserver.get());
    }

    void PeerConnection::OnIceConnectionChange(const webrtc::PeerConnectionInterface::IceConnectionState new_state) {
        auto newValue = IceState::Unknown;
        switch (new_state) {
            case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionNew:
                newValue = IceState::New;
                break;
            case webrtc::PeerConnectionInterface::kIceConnectionChecking:
                newValue = IceState::Checking;
                break;
            case webrtc::PeerConnectionInterface::kIceConnectionConnected:
                newValue = IceState::Connected;
                break;
            case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
                newValue = IceState::Completed;
                break;
            case webrtc::PeerConnectionInterface::kIceConnectionFailed:
            case webrtc::PeerConnectionInterface::kIceConnectionMax:
                newValue = IceState::Failed;
                break;
            case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
                newValue = IceState::Disconnected;
                break;
            case webrtc::PeerConnectionInterface::kIceConnectionClosed:
                newValue = IceState::Closed;
                break;
        }
        (void) stateChangeCallback(newValue);
    }

    void PeerConnection::OnIceGatheringChange(const webrtc::PeerConnectionInterface::IceGatheringState new_state) {
        auto newValue = GatheringState::Unknown;
        switch (new_state) {
            case webrtc::PeerConnectionInterface::kIceGatheringNew:
                newValue = GatheringState::New;
                break;
            case webrtc::PeerConnectionInterface::kIceGatheringGathering:
                newValue = GatheringState::InProgress;
                break;
            case webrtc::PeerConnectionInterface::kIceGatheringComplete:
                newValue = GatheringState::Complete;
                break;
        }
        (void) gatheringStateChangeCallback(newValue);
    }

    void PeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {
        iceCandidateCallback(IceCandidate(candidate));
    }

    void PeerConnection::OnRenegotiationNeeded() {
        (void) renegotiationNeededCallback();
    }

    void PeerConnection::OnDataChannel(const rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
        if (allowAttachDataChannel && !dataChannel) {
            attachDataChannel(data_channel);
        }
    }

    void PeerConnection::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {

    }

    void PeerConnection::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {

    }

    void PeerConnection::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) {

    }

    void PeerConnection::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {

    }

    void PeerConnection::OnConnectionChange(const webrtc::PeerConnectionInterface::PeerConnectionState newState) {
        auto newValue = ConnectionState::Unknown;
        switch (newState) {
            case webrtc::PeerConnectionInterface::PeerConnectionState::kNew:
                newValue = ConnectionState::New;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting:
                newValue = ConnectionState::Connecting;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
                newValue = ConnectionState::Connected;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
                newValue = ConnectionState::Disconnected;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kFailed:
                newValue = ConnectionState::Failed;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
                newValue = ConnectionState::Closed;
                break;
        }
        (void) connectionChangeCallback(newValue, alreadyConnected);
        if (newValue == ConnectionState::Connected && !alreadyConnected) {
            alreadyConnected = true;
        }
    }
} // wrtc