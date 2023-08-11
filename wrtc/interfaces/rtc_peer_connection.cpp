//
// Created by Laky64 on 08/08/2023.
//

#include "rtc_peer_connection.hpp"

namespace wrtc {
    RTCPeerConnection::RTCPeerConnection() {
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
                configuration, std::move(dependencies));
        if (!result.ok()) {
            throw wrapRTCError(result.error());
        }
        _jinglePeerConnection = result.MoveValue();
    }

    RTCPeerConnection::~RTCPeerConnection() {
        _jinglePeerConnection = nullptr;
        if (_factory) {
            if (_shouldReleaseFactory) {
                PeerConnectionFactory::Release();
            }
            _factory = nullptr;
        }
    }

    Description RTCPeerConnection::CreateOffer(bool offerToReceiveAudio, bool offerToReceiveVideo) {
        if (!_jinglePeerConnection ||
            _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw BaseRTCException("Failed to execute 'createOffer' on 'RTCPeerConnection': The RTCPeerConnection's signalingState is 'closed'.");
        }

        Sync<Description> description;
        auto observer = new rtc::RefCountedObject<CreateSessionDescriptionObserver>(description.onSuccess, description.onFailed);

        auto options = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
        options.offer_to_receive_audio = offerToReceiveAudio;
        options.offer_to_receive_video = offerToReceiveVideo;
        _jinglePeerConnection->CreateOffer(observer, options);

        return description.get();
    }

    void RTCPeerConnection::SetLocalDescription(Description &description) {
        auto *raw_description = static_cast<webrtc::SessionDescriptionInterface *>(description);
        std::unique_ptr<webrtc::SessionDescriptionInterface> raw_description_ptr(raw_description);

        if (!_jinglePeerConnection ||
            _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setLocalDescription' on 'RTCPeerConnection': The RTCPeerConnection's signalingState is 'closed'.");
        }

        Sync<void> future;
        auto observer = new rtc::RefCountedObject<SetSessionDescriptionObserver>(future.onSuccess, future.onFailed);
        _jinglePeerConnection->SetLocalDescription(observer, raw_description_ptr.release());

        future.wait();
    }

    void RTCPeerConnection::SetRemoteDescription(Description &description) {
        auto *raw_description = static_cast<webrtc::SessionDescriptionInterface *>(description);
        std::unique_ptr<webrtc::SessionDescriptionInterface> raw_description_ptr(raw_description);

        if (!_jinglePeerConnection ||
            _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            throw RTCException("Failed to execute 'setRemoteDescription' on 'RTCPeerConnection': The RTCPeerConnection's signalingState is 'closed'.");
        }

        Sync<void> future;
        auto observer = new rtc::RefCountedObject<SetSessionDescriptionObserver>(future.onSuccess, future.onFailed);
        _jinglePeerConnection->SetRemoteDescription(observer, raw_description_ptr.release());

        future.wait();
    }

    RTCRtpSender *RTCPeerConnection::AddTrack(
            MediaStreamTrack &mediaStreamTrack, const std::vector<MediaStream *> &mediaStreams) {
        if (!_jinglePeerConnection) {
            throw RTCException("Cannot add track; RTCPeerConnection is closed");
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

    RTCRtpSender *RTCPeerConnection::AddTrack(
            MediaStreamTrack &mediaStreamTrack, std::optional<std::reference_wrapper<MediaStream>> mediaStream) {
        if (!_jinglePeerConnection) {
            throw RTCException("Cannot add track; RTCPeerConnection is closed");
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

    void RTCPeerConnection::RestartIce() {
        if (_jinglePeerConnection) {
            _jinglePeerConnection->RestartIce();
        }
    }

    void RTCPeerConnection::Close() {
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

    void RTCPeerConnection::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
        if (new_state == webrtc::PeerConnectionInterface::kClosed) {
        }
    }

    void RTCPeerConnection::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {

    }

    void RTCPeerConnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {

    }

    void RTCPeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {

    }

    void RTCPeerConnection::OnIceCandidateError(const std::string &host_candidate, const std::string &url, int error_code,
                                                const std::string &error_text) {

    }

    void RTCPeerConnection::OnRenegotiationNeeded() {

    }

    void RTCPeerConnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {

    }

    void RTCPeerConnection::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {

    }

    void RTCPeerConnection::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {

    }

    void RTCPeerConnection::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                                       const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) {

    }

    void RTCPeerConnection::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {

    }
}