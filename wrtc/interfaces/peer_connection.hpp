//
// Created by Laky64 on 16/08/2023.
//

#pragma once

#include <optional>
#include <api/peer_connection_interface.h>

#include "../enums.hpp"
#include "../exceptions.hpp"
#include "../utils/sync.hpp"
#include "../utils/syncronized_callback.hpp"
#include "../models/rtc_session_description.hpp"
#include "media/tracks/media_stream_track.hpp"
#include "peer_connection/peer_connection_factory.hpp"
#include "peer_connection/create_session_description_observer.hpp"
#include "peer_connection/set_session_description_observer.hpp"

namespace wrtc {

    class PeerConnection: public webrtc::PeerConnectionObserver {
    public:
        PeerConnection();

        ~PeerConnection() override;

        Description createOffer(bool offerToReceiveAudio = true, bool offerToReceiveVideo = false);

        void setLocalDescription(Description &description);

        void setRemoteDescription(Description &description);

        void addTrack(MediaStreamTrack *mediaStreamTrack, std::vector<std::string> streamIds = {});

        void restartIce();

        void close();

        void onIceStateChange(const std::function<void(IceState state)> &callback);

        void onGatheringStateChange(const std::function<void(GatheringState state)> &callback);

        void onSignalingStateChange(const std::function<void(SignalingState state)> &callback);

    private:
        rtc::scoped_refptr<PeerConnectionFactory> factory;
        rtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection;

        synchronized_callback<IceState> stateChangeCallback;
        synchronized_callback<GatheringState> gatheringStateChangeCallback;
        synchronized_callback<SignalingState> signalingStateChangeCallback;


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
    };

} // wrtc
