//
// Created by Laky64 on 06/10/24.
//

#include <wrtc/interfaces/wrapped_dtls_srtp_transport.hpp>

namespace wrtc {
    WrappedDtlsSrtpTransport::WrappedDtlsSrtpTransport(
        const bool rtcpMuxEnabled,
        const webrtc::FieldTrialsView& field_trials,
        const std::function<void(webrtc::RtpPacketReceived)>& callback
    ): DtlsSrtpTransport(rtcpMuxEnabled, field_trials) {
        rtpPacketCallback = callback;
    }

    void WrappedDtlsSrtpTransport::OnRtpPacketReceived(const rtc::ReceivedPacket& packet) {
        rtc::CopyOnWriteBuffer payload(packet.payload());

        webrtc::RtpPacketReceived parsedPacket(&headerExtensionMap);
        parsedPacket.set_arrival_time(packet.arrival_time().value_or(webrtc::Timestamp::MinusInfinity()));
        parsedPacket.set_ecn(packet.ecn());

        if (parsedPacket.Parse(std::move(payload))) {
            (void) rtpPacketCallback(parsedPacket);
        }
    }

    void WrappedDtlsSrtpTransport::UpdateRtpHeaderExtensionMap(const cricket::RtpHeaderExtensions& headerExtensions) {
        headerExtensionMap = webrtc::RtpHeaderExtensionMap(headerExtensions);
        DtlsSrtpTransport::UpdateRtpHeaderExtensionMap(headerExtensions);
    }
} // wrtc