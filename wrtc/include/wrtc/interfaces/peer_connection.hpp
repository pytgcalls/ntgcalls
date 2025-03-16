//
// Created by Laky64 on 16/08/2023.
//

#pragma once

#include <optional>
#include <api/peer_connection_interface.h>

#include <wrtc/enums.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/models/ice_candidate.hpp>
#include <wrtc/utils/syncronized_callback.hpp>
#include <wrtc/interfaces/network_interface.hpp>
#include <wrtc/models/rtc_session_description.hpp>
#include <wrtc/interfaces/peer_connection/data_channel_observer_impl.hpp>

namespace wrtc {

    class PeerConnection final : public webrtc::PeerConnectionObserver, public NetworkInterface {
    public:
        explicit PeerConnection(const webrtc::PeerConnectionInterface::IceServers& servers = {}, bool allowAttachDataChannel = false, bool allowP2P = true);

        void open() override{}

        std::optional<Description> localDescription() const;

        void setLocalDescription(const std::function<void()> &onSuccess, const std::function<void(const std::exception_ptr&)> &onError) const;

        void setLocalDescription() const;

        void setRemoteDescription(const Description &description, const std::function<void()> &onSuccess, const std::function<void(const std::exception_ptr&)> &onError) const;

        void setRemoteDescription(const Description &description) const;

        std::unique_ptr<MediaTrackInterface> addOutgoingTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) override;

        void addIceCandidate(const IceCandidate& rawCandidate) const override;

        void restartIce() const;

        void createDataChannel(const std::string &label);

        void sendDataChannelMessage(const bytes::binary &data) const override;

        void close() override;

        SignalingState signalingState() const;

        void onIceStateChange(const std::function<void(IceState state)> &callback);

        void onGatheringStateChange(const std::function<void(GatheringState state)> &callback);

        void onSignalingStateChange(const std::function<void(SignalingState state)> &callback);

        void onRenegotiationNeeded(const std::function<void()> &callback);

        void onDataChannelMessage(const std::function<void(bytes::binary)> &callback);


        void addIncomingAudioTrack(const std::weak_ptr<RemoteAudioSink>& sink) override {}
        void addIncomingVideoTrack(const std::weak_ptr<RemoteVideoSink>& sink, bool isScreenCast) override {}

    private:
        rtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection;
        rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel;
        std::unique_ptr<DataChannelObserverImpl> dataChannelObserver;

        synchronized_callback<IceState> stateChangeCallback;
        synchronized_callback<GatheringState> gatheringStateChangeCallback;
        synchronized_callback<void> renegotiationNeededCallback;
        synchronized_callback<bytes::binary> dataChannelMessageCallback;
        synchronized_callback<SignalingState> signalingStateChangeCallback;
        bool allowAttachDataChannel = false;

        // PeerConnectionObserver implementation.
        void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;

        void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

        void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

        void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;

        void OnRenegotiationNeeded() override;

        void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

        void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

        void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

        void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) override;

        void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;

        void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState) override;

        static SignalingState parseSignalingState(webrtc::PeerConnectionInterface::SignalingState state);

        void onDataChannelStateUpdated();

        void attachDataChannel(const rtc::scoped_refptr<webrtc::DataChannelInterface>& dataChannel);
    };

} // wrtc
