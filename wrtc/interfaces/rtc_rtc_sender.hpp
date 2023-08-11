//
// Created by Laky64 on 09/08/2023.
//

#pragma once

#include "rtc_peer_connection/peer_connection_factory.hpp"
#include "../utils/instance_holder.hpp"
#include "media_stream_track.hpp"
#include "rtc_dtls_transport.hpp"

namespace wrtc {
    class RTCRtpSender {
    public:
        explicit RTCRtpSender(PeerConnectionFactory *, rtc::scoped_refptr<webrtc::RtpSenderInterface>);

        static RTCRtpSender *Create(PeerConnectionFactory *, rtc::scoped_refptr<webrtc::RtpSenderInterface>);

        ~RTCRtpSender();

        static InstanceHolder<
                RTCRtpSender *, rtc::scoped_refptr<webrtc::RtpSenderInterface>, PeerConnectionFactory *
        > *holder();

        std::optional<MediaStreamTrack *> GetTrack();

        std::optional<RTCDtlsTransport *> GetTransport();

        rtc::scoped_refptr<webrtc::RtpSenderInterface> sender() { return _sender; }

    private:
        PeerConnectionFactory *_factory;
        rtc::scoped_refptr<webrtc::RtpSenderInterface> _sender;
    };
} // wrtc
