//
// Created by Laky64 on 16/08/2023.
//

#include "peer_connection.hpp"

#include <iostream>

#include "../utils/sync.hpp"
#include "peer_connection/create_session_description_observer.hpp"
#include "peer_connection/set_session_description_observer.hpp"
#include "api/jsep_ice_candidate.h"

namespace wrtc {

    PeerConnection::PeerConnection(const std::vector<RTCServer>& servers) {
        factory = PeerConnectionFactory::GetOrCreateDefault();

        webrtc::PeerConnectionInterface::RTCConfiguration config;
        config.type = webrtc::PeerConnectionInterface::IceTransportsType::kAll;
        config.bundle_policy = webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxBundle;
        config.servers = RTCServer::toIceServers(servers);
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

    PeerConnection::~PeerConnection() {
        close();
    }

    Description PeerConnection::createOffer(const bool offerToReceiveAudio, const bool offerToReceiveVideo) const {
        if (!peerConnection ||
            peerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'createOffer' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }
        Sync<std::optional<Description>> description;
        const auto observer = new rtc::RefCountedObject<CreateSessionDescriptionObserver>(description.onSuccess, description.onFailed);
        auto options = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
        options.offer_to_receive_audio = offerToReceiveAudio;
        options.offer_to_receive_video = offerToReceiveVideo;
        peerConnection->CreateOffer(observer, options);
        return description.get();
    }

    std::optional<Description> PeerConnection::localDescription() const {
        if (peerConnection) {
            if (const auto raw_description = peerConnection->local_description()) {
                return Description::Wrap(raw_description);
            }
        }
        return std::nullopt;
    }

    void PeerConnection::setLocalDescription(const std::optional<Description>& description, const std::function<void()>& onSuccess) const {
        if (!peerConnection ||
            peerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setLocalDescription' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }

        Sync<void> future;
        const rtc::scoped_refptr<webrtc::SetLocalDescriptionObserverInterface> observer(new rtc::RefCountedObject<SetSessionDescriptionObserver>(
            onSuccess ? onSuccess : future.onSuccess,
            future.onFailed
        ));
        if (description) {
            webrtc::SdpParseError sdpParseError;
            std::unique_ptr<webrtc::SessionDescriptionInterface> localDescription(
                CreateSessionDescription(Description::typeToString(description.value().getType()), description.value().getSdp(), &sdpParseError)
            );
            if (!localDescription) {
                throw wrapSdpParseError(sdpParseError);
            }
            peerConnection->SetLocalDescription(std::move(localDescription), observer);
        } else {
            peerConnection->SetLocalDescription(observer);
        }
        if (!onSuccess) {
            future.wait();
        }
    }

    void PeerConnection::setRemoteDescription(const Description &description, const std::function<void()>& onSuccess) const {
        webrtc::SdpParseError sdpParseError;
        std::unique_ptr<webrtc::SessionDescriptionInterface> remoteDescription(
            CreateSessionDescription(Description::typeToString(description.getType()), description.getSdp(), &sdpParseError)
        );
        if (!remoteDescription) {
            throw wrapSdpParseError(sdpParseError);
        }
        if (!peerConnection ||
            peerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setRemoteDescription' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }

        Sync<void> future;
        const rtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface> observer(new rtc::RefCountedObject<SetSessionDescriptionObserver>(
            onSuccess ? onSuccess : future.onSuccess,
            future.onFailed
        ));
        peerConnection->SetRemoteDescription(std::move(remoteDescription), observer);
        if (!onSuccess) {
            future.wait();
        }
    }

    void PeerConnection::addIceCandidate(const IceCandidate& rawCandidate) const {
        webrtc::SdpParseError error;
        const auto candidate = CreateIceCandidate(rawCandidate.mid, rawCandidate.mLine, rawCandidate.sdp, &error);
        if (!candidate) {
            throw wrapSdpParseError(error);
        }
        peerConnection->AddIceCandidate(candidate);
    }

    void PeerConnection::addTrack(MediaStreamTrack *mediaStreamTrack, const std::vector<std::string>& streamIds) const
    {
        if (!peerConnection) {
            throw RTCException("Cannot add track; PeerConnection is closed");
        }
        if (const auto result = peerConnection->AddTrack(mediaStreamTrack->track(), streamIds); !result.ok()) {
            throw wrapRTCError(result.error());
        }
    }

    void PeerConnection::restartIce() const
    {
        if (peerConnection) {
            peerConnection->RestartIce();
        }
    }

    void PeerConnection::close() {
        if (peerConnection) {
            peerConnection->Close();
            if (peerConnection->GetConfiguration().sdp_semantics == webrtc::SdpSemantics::kUnifiedPlan) {
                for (const auto &transceiver: peerConnection->GetTransceivers()) {
                    const auto track = MediaStreamTrack::holder()->GetOrCreate(transceiver->receiver()->track());
                    track->OnPeerConnectionClosed();
                }
            }
            peerConnection = nullptr;
            if (factory) {
                PeerConnectionFactory::UnRef();
                factory = nullptr;
            }
        }
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

    void PeerConnection::onIceCandidate(const std::function<void(const IceCandidate& candidate)>& callback) {
        iceCandidateCallback = callback;
    }

    void PeerConnection::onRenegotiationNeeded(const std::function<void()>& callback) {
        renegotiationNeedeCallbackd = callback;
    }

    rtc::Thread* PeerConnection::networkThread() const {
        return factory->networkThread();
    }

    rtc::Thread* PeerConnection::signalingThread() const {
        return factory->signalingThread();
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
        (void) renegotiationNeedeCallbackd();
    }

    void PeerConnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {

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

    void PeerConnection::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState) {
        switch (newState) {
            case webrtc::PeerConnectionInterface::PeerConnectionState::kNew:
                std::cout << "New" << std::endl;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting:
                std::cout << "Connecting" << std::endl;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
                std::cout << "Connected" << std::endl;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
                std::cout << "Disconnected" << std::endl;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kFailed:
                std::cout << "Failed" << std::endl;
                break;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
                std::cout << "Closed" << std::endl;
                break;
        }
    }
} // wrtc