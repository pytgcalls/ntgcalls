//
// Created by Laky64 on 06/10/24.
//

#include <wrtc/interfaces/wrapped_dtls_srtp_transport.hpp>
#include <modules/rtp_rtcp/source/rtp_util.h>

namespace wrtc {
    WrappedDtlsSrtpTransport::WrappedDtlsSrtpTransport(
        const bool rtcpMuxEnabled,
        const webrtc::FieldTrialsView& field_trials,
        const std::function<void(webrtc::RtpPacketReceived)>& callback
    ): DtlsSrtpTransport(rtcpMuxEnabled, field_trials) {
        rtpPacketCallback = callback;
    }

    void WrappedDtlsSrtpTransport::OnRtpPacketReceived(const rtc::ReceivedPacket& packet) {
        if (!IsSrtpActive()) {
            RTC_LOG(LS_WARNING) << "Inactive SRTP transport received an RTP packet. Drop it.";
            return;
        }

        rtc::CopyOnWriteBuffer payload(packet.payload());
        char* data = payload.MutableData<char>();
        int len = rtc::checked_cast<int>(payload.size());
        if (!UnprotectRtp(data, len, &len)) {
            if (decryptionFailureCount % 100 == 0) {
                RTC_LOG(LS_ERROR) << "Failed to unprotect RTP packet: size=" << len
                                  << ", seqnum=" << webrtc::ParseRtpSequenceNumber(payload)
                                  << ", SSRC=" << webrtc::ParseRtpSsrc(payload)
                                  << ", previous failure count: "
                                  << decryptionFailureCount;
            }
            ++decryptionFailureCount;
            return;
        }
        payload.SetSize(len);

        webrtc::RtpPacketReceived parsedPacket(&headerExtensionMap);
        parsedPacket.set_arrival_time(packet.arrival_time().value_or(webrtc::Timestamp::MinusInfinity()));
        parsedPacket.set_ecn(packet.ecn());

        if (parsedPacket.Parse(payload)) {
            (void) rtpPacketCallback(parsedPacket);
        }

        DemuxPacket(payload, packet.arrival_time().value_or(webrtc::Timestamp::MinusInfinity()), packet.ecn());
    }

    void WrappedDtlsSrtpTransport::UpdateRtpHeaderExtensionMap(const cricket::RtpHeaderExtensions& headerExtensions) {
        headerExtensionMap = webrtc::RtpHeaderExtensionMap(headerExtensions);
        DtlsSrtpTransport::UpdateRtpHeaderExtensionMap(headerExtensions);
    }
} // wrtc