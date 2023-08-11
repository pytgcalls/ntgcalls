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

    class RTCPeerConnection : public webrtc::PeerConnectionObserver {
    private:
        rtc::scoped_refptr<webrtc::PeerConnectionInterface> _jinglePeerConnection;
        PeerConnectionFactory *_factory;
        bool _shouldReleaseFactory;

    public:
        explicit RTCPeerConnection();

        ~RTCPeerConnection() override;

        Description CreateOffer(bool offerToReceiveAudio = false, bool offerToReceiveVideo = false);

        void SetLocalDescription(Description &description);

        void SetRemoteDescription(Description &description);

        RTCRtpSender *AddTrack(MediaStreamTrack &mediaStreamTrack, const std::vector<MediaStream *> &mediaStreams);

        RTCRtpSender *AddTrack(MediaStreamTrack &mediaStreamTrack, std::optional<std::reference_wrapper<MediaStream>> mediaStream);

        void RestartIce();

        void Close();

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
    };
}
