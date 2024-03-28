//
// Created by Laky64 on 16/08/2023.
//

#pragma once

#include <optional>
#include <api/peer_connection_interface.h>
#include "../enums.hpp"
#include "../exceptions.hpp"
#include "../utils/syncronized_callback.hpp"
#include "../models/rtc_session_description.hpp"
#include "media/tracks/media_stream_track.hpp"
#include "peer_connection/peer_connection_factory.hpp"
#include "wrtc/models/ice_candidate.hpp"
#include "../../ntgcalls/models/rtc_server.hpp"
#include "peer_connection/data_channel_observer_impl.hpp"

namespace wrtc {

    class PeerConnection final : public webrtc::PeerConnectionObserver {
    public:
        explicit PeerConnection(const std::vector<RTCServer>& servers = {}, bool allowAttachDataChannel = false, bool p2pAllowed = true);

        ~PeerConnection() override;

        std::optional<Description> localDescription() const;

        void setLocalDescription(const std::function<void()> &onSuccess, const std::function<void(const std::exception_ptr&)> &onError) const;

        void setLocalDescription() const;

        void setRemoteDescription(const Description &description, const std::function<void()> &onSuccess, const std::function<void(const std::exception_ptr&)> &onError) const;

        void setRemoteDescription(const Description &description) const;

        void addTrack(MediaStreamTrack *mediaStreamTrack, const std::vector<std::string>& streamIds = {}) const;

        void addIceCandidate(const IceCandidate& rawCandidate) const;

        void restartIce() const;

        void createDataChannel(const std::string &label);

        void sendDataChannelMessage(const bytes::binary &data) const;

        void close();

        SignalingState signalingState() const;

        void onIceStateChange(const std::function<void(IceState state)> &callback);

        void onGatheringStateChange(const std::function<void(GatheringState state)> &callback);

        void onSignalingStateChange(const std::function<void(SignalingState state)> &callback);

        void onIceCandidate(const std::function<void(const IceCandidate &candidate)> &callback);

        void onRenegotiationNeeded(const std::function<void()> &callback);

        void onConnectionChange(const std::function<void(PeerConnectionState state)> &callback);

        void onDataChannelOpened(const std::function<void()> &callback);

        void onDataChannelMessage(const std::function<void(bytes::binary)> &callback);

        rtc::Thread *networkThread() const;

        rtc::Thread *signalingThread() const;

        bool isDataChannelOpen() const;

    private:
        rtc::scoped_refptr<PeerConnectionFactory> factory;
        rtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection;
        rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel;
        std::unique_ptr<DataChannelObserverImpl> dataChannelObserver;

        synchronized_callback<IceState> stateChangeCallback;
        synchronized_callback<GatheringState> gatheringStateChangeCallback;
        synchronized_callback<void> renegotiationNeededCallback;
        synchronized_callback<void> dataChannelOpenedCallback;
        synchronized_callback<bytes::binary> dataChannelMessageCallback;
        synchronized_callback<SignalingState> signalingStateChangeCallback;
        synchronized_callback<PeerConnectionState> connectionChangeCallback;
        synchronized_callback<IceCandidate> iceCandidateCallback;
        bool allowAttachDataChannel, dataChannelOpen = false;


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
