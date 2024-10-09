//
// Created by Laky64 on 06/10/24.
//

#pragma once
#include <pc/dtls_srtp_transport.h>
#include <wrtc/utils/syncronized_callback.hpp>

namespace wrtc {

    class WrappedDtlsSrtpTransport final : public webrtc::DtlsSrtpTransport {
        webrtc::RtpHeaderExtensionMap headerExtensionMap;
        synchronized_callback<webrtc::RtpPacketReceived> rtpPacketCallback;
        int decryptionFailureCount = 0;

    public:
        WrappedDtlsSrtpTransport(
            bool rtcpMuxEnabled,
            const webrtc::FieldTrialsView& field_trials,
            const std::function<void(webrtc::RtpPacketReceived)>& callback
        );

        void OnRtpPacketReceived(const rtc::ReceivedPacket& packet) override;

        void UpdateRtpHeaderExtensionMap(const cricket::RtpHeaderExtensions& headerExtensions) override;
    };

} // wrtc
