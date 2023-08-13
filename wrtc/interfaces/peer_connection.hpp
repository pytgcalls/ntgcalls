//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/p2p/client/basic_port_allocator.h>


#include "../exceptions.hpp"
#include "../models/rtc_session_description.hpp"
#include "../utils/sync.hpp"
#include "../utils/syncronized_callback.hpp"
#include "../enums.hpp"
#include "rtc_peer_connection/peer_connection_factory.hpp"
#include "rtc_peer_connection/create_session_description_observer.hpp"
#include "rtc_peer_connection/set_session_description_observer.hpp"
#include "rtc_rtc_sender.hpp"
#include "media_stream.hpp"

namespace webrtc {
    struct PeerConnectionDependencies;
}

namespace wrtc {
    class PeerConnectionFactory;

    class PeerConnection : public webrtc::PeerConnectionObserver {
    private:
        rtc::scoped_refptr<webrtc::PeerConnectionInterface> _jinglePeerConnection;
        PeerConnectionFactory *_factory;
        bool _shouldReleaseFactory;

        synchronized_callback<IceState> stateChangeCallback;
        synchronized_callback<GatheringState> gatheringStateChangeCallback;
        synchronized_callback<SignalingState> signalingStateChangeCallback;


        // PeerConnectionObserver implementation.
        void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;

        void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

        void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

        void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;

        void OnIceCandidateError(const std::string &host_candidate, const std::string &url, int error_code,
                                 const std::string &error_text) override;

        void OnRenegotiationNeeded() override;

        void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

        void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

        void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

        void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) override;

        void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;

    public:
        explicit PeerConnection();

        ~PeerConnection() override;

        Description createOffer(bool offerToReceiveAudio = false, bool offerToReceiveVideo = false);

        void setLocalDescription(Description &description);

        void setRemoteDescription(Description &description);

        RTCRtpSender *addTrack(MediaStreamTrack *mediaStreamTrack);

        void restartIce();

        void close();

        void onIceStateChange(const std::function<void(IceState state)> &callback);

        void onGatheringStateChange(const std::function<void(GatheringState state)> &callback);

        void onSignalingStateChange(const std::function<void(SignalingState state)> &callback);
    };
}
