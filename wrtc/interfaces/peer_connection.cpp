//
// Created by Laky64 on 16/08/2023.
//

#include "peer_connection.hpp"
#include "../utils/sync.hpp"
#include "peer_connection/create_session_description_observer.hpp"
#include "peer_connection/set_session_description_observer.hpp"
#include "api/jsep_ice_candidate.h"

namespace wrtc {

    PeerConnection::PeerConnection(const webrtc::PeerConnectionInterface::IceServers& servers) {
        factory = PeerConnectionFactory::GetOrCreateDefault();

        webrtc::PeerConnectionInterface::RTCConfiguration config;
        config.bundle_policy = webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxBundle;
        config.servers = servers;

        webrtc::PeerConnectionDependencies dependencies(this);

        auto result = factory->factory()->CreatePeerConnectionOrError(
                config, std::move(dependencies));


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

    void PeerConnection::setLocalDescription(const std::optional<Description>& description) const {
        if (!peerConnection ||
            peerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setLocalDescription' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }

        Sync<void> future;
        const auto observer = new rtc::RefCountedObject<SetSessionDescriptionObserver>(future.onSuccess, future.onFailed);
        if (!description) {
            auto *raw_description = static_cast<webrtc::SessionDescriptionInterface *>(description.value());
            std::unique_ptr<webrtc::SessionDescriptionInterface> raw_description_ptr(raw_description);
            peerConnection->SetLocalDescription(observer, raw_description_ptr.release());
        } else {
            peerConnection->SetLocalDescription(observer);
        }
        future.wait();
    }

    void PeerConnection::setRemoteDescription(const Description &description) const {
        auto *raw_description = static_cast<webrtc::SessionDescriptionInterface *>(description);
        std::unique_ptr<webrtc::SessionDescriptionInterface> raw_description_ptr(raw_description);

        if (!peerConnection ||
            peerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setRemoteDescription' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }

        Sync<void> future;
        const auto observer = new rtc::RefCountedObject<SetSessionDescriptionObserver>(future.onSuccess, future.onFailed);
        peerConnection->SetRemoteDescription(observer, raw_description_ptr.release());

        future.wait();
    }

    void PeerConnection::addIceCandidate(const std::string &rawCandidate) const {
        webrtc::JsepIceCandidate parseCandidate{ std::string(), 0 };
        if (!parseCandidate.Initialize(rawCandidate, nullptr)) {
            throw RTCException("Failed to parse candidate");
        }
        const auto candidate = webrtc::CreateIceCandidate(parseCandidate.sdp_mid(), parseCandidate.sdp_mline_index(), parseCandidate.candidate());
        if (!candidate) {
            throw RTCException("Failed to parse candidate");
        }
        peerConnection->AddIceCandidate(candidate.get());
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

    rtc::Thread* PeerConnection::networkThread() const {
        return factory->networkThread();
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

    }

    void PeerConnection::OnRenegotiationNeeded() {

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
} // wrtc