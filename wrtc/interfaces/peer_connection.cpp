//
// Created by Laky64 on 08/08/2023.
//

#include "peer_connection.hpp"

namespace wrtc {
    PeerConnection::PeerConnection() {
        _factory = PeerConnectionFactory::GetOrCreateDefault();
        _shouldReleaseFactory = true;
        auto configuration = webrtc::PeerConnectionInterface::RTCConfiguration();
        configuration.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

        auto portAllocator = std::unique_ptr<cricket::PortAllocator>(new cricket::BasicPortAllocator(
                _factory->getNetworkManager(),
                _factory->getSocketFactory())
        );

        portAllocator->SetPortRange(0, 65535);
        webrtc::PeerConnectionDependencies dependencies(this);
        dependencies.allocator = std::move(portAllocator);
        auto result = _factory->factory()->CreatePeerConnectionOrError(
            configuration,
            std::move(dependencies)
        );
        if (!result.ok()) {
            throw wrapRTCError(result.error());
        }
        _jinglePeerConnection = result.MoveValue();
    }

    PeerConnection::~PeerConnection() {
        _jinglePeerConnection = nullptr;
        if (_factory) {
            if (_shouldReleaseFactory) {
                PeerConnectionFactory::Release();
            }
            _factory = nullptr;
        }
    }

    Description PeerConnection::createOffer(bool offerToReceiveAudio, bool offerToReceiveVideo) {
        if (!_jinglePeerConnection ||
            _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw BaseRTCException("Failed to execute 'createOffer' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }

        Sync<std::optional<Description>> description;
        auto observer = new rtc::RefCountedObject<CreateSessionDescriptionObserver>(description.onSuccess, description.onFailed);

        auto options = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
        options.offer_to_receive_audio = offerToReceiveAudio;
        options.offer_to_receive_video = offerToReceiveVideo;
        _jinglePeerConnection->CreateOffer(observer, options);

        return description.get();
    }

    void PeerConnection::setLocalDescription(Description &description) {
        auto *raw_description = static_cast<webrtc::SessionDescriptionInterface *>(description);
        std::unique_ptr<webrtc::SessionDescriptionInterface> raw_description_ptr(raw_description);

        if (!_jinglePeerConnection ||
            _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setLocalDescription' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }

        Sync<void> future;
        auto observer = new rtc::RefCountedObject<SetSessionDescriptionObserver>(future.onSuccess, future.onFailed);
        _jinglePeerConnection->SetLocalDescription(observer, raw_description_ptr.release());

        future.wait();
    }

    void PeerConnection::setRemoteDescription(Description &description) {
        auto *raw_description = static_cast<webrtc::SessionDescriptionInterface *>(description);
        std::unique_ptr<webrtc::SessionDescriptionInterface> raw_description_ptr(raw_description);

        if (!_jinglePeerConnection ||
            _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setRemoteDescription' on 'PeerConnection': The PeerConnection's signalingState is 'closed'.");
        }

        Sync<void> future;
        auto observer = new rtc::RefCountedObject<SetSessionDescriptionObserver>(future.onSuccess, future.onFailed);
        _jinglePeerConnection->SetRemoteDescription(observer, raw_description_ptr.release());

        future.wait();
    }

    RTCRtpSender *PeerConnection::addTrack(
            MediaStreamTrack &mediaStreamTrack, const std::vector<MediaStream *> &mediaStreams) {
        if (!_jinglePeerConnection) {
            throw RTCException("Cannot add track; PeerConnection is closed");
        }

        std::vector<std::string> streamIds;
        streamIds.reserve(mediaStreams.size());
        for (auto const &stream: mediaStreams) {
            streamIds.emplace_back(stream->stream()->id());
        }

        auto result = _jinglePeerConnection->AddTrack(mediaStreamTrack.track(), streamIds);
        if (!result.ok()) {
            throw wrapRTCError(result.error());
        }

        auto rtpSender = result.value();
        return RTCRtpSender::holder()->GetOrCreate(_factory, rtpSender);
    }

    RTCRtpSender *PeerConnection::addTrack(
            MediaStreamTrack &mediaStreamTrack, std::optional<std::reference_wrapper<MediaStream>> mediaStream) {
        if (!_jinglePeerConnection) {
            throw RTCException("Cannot add track; PeerConnection is closed");
        }

        std::vector<std::string> streamIds;
        if (mediaStream != std::nullopt) {
            streamIds.emplace_back(mediaStream->get().stream()->id());
        }

        auto result = _jinglePeerConnection->AddTrack(mediaStreamTrack.track(), streamIds);
        if (!result.ok()) {
            throw wrapRTCError(result.error());
        }

        auto rtpSender = result.value();
        return RTCRtpSender::holder()->GetOrCreate(_factory, rtpSender);
    }

    void PeerConnection::restartIce() {
        if (_jinglePeerConnection) {
            _jinglePeerConnection->RestartIce();
        }
    }

    void PeerConnection::close() {
        if (_jinglePeerConnection) {
            _jinglePeerConnection->Close();

            if (_jinglePeerConnection->GetConfiguration().sdp_semantics == webrtc::SdpSemantics::kUnifiedPlan) {
                for (const auto &transceiver: _jinglePeerConnection->GetTransceivers()) {
                    auto track = MediaStreamTrack::holder()->GetOrCreate(_factory, transceiver->receiver()->track());
                    track->OnPeerConnectionClosed();
                }
            }
        }

        _jinglePeerConnection = nullptr;

        if (_factory) {
            if (_shouldReleaseFactory) {
                PeerConnectionFactory::Release();
            }
            _factory = nullptr;
        }
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

    void PeerConnection::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
        SignalingState newValue;
        switch (new_state) {
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
                return;
        }
        signalingStateChangeCallback(newValue);
    }

    void PeerConnection::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
        IceState newValue;
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
        stateChangeCallback(newValue);
    }

    void PeerConnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
        GatheringState newValue;
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
        gatheringStateChangeCallback(newValue);
    }

    void PeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {

    }

    void PeerConnection::OnIceCandidateError(const std::string &host_candidate, const std::string &url, int error_code,
                                                const std::string &error_text) {

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
}