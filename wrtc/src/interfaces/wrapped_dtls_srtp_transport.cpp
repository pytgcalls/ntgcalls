//
// Created by Laky64 on 06/10/24.
//

#include <wrtc/interfaces/wrapped_dtls_srtp_transport.hpp>
#include <modules/rtp_rtcp/source/rtp_util.h>

namespace wrtc {
    template <typename Tag, typename Tag::pfn_t pfn>
    struct tag_bind_pfn {
        friend constexpr typename Tag::pfn_t pfn_of(Tag) { return pfn; }
    };

    struct tag_SrtpTransport_UnprotectRtp {
        using pfn_t = bool (webrtc::SrtpTransport::*)(rtc::CopyOnWriteBuffer& buffer);
        friend constexpr pfn_t pfn_of(tag_SrtpTransport_UnprotectRtp);
    };
    template struct tag_bind_pfn<tag_SrtpTransport_UnprotectRtp, &webrtc::SrtpTransport::UnprotectRtp>;

    inline static auto c_pfn_SrtpTransport_UnprotectRtp = pfn_of(tag_SrtpTransport_UnprotectRtp{});

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
        if (!(this->*c_pfn_SrtpTransport_UnprotectRtp)(payload)) {
            if (decryptionFailureCount % 100 == 0) {
                RTC_LOG(LS_ERROR) << "Failed to unprotect RTP packet: size=" << payload.size()
                                  << ", seqnum=" << webrtc::ParseRtpSequenceNumber(payload)
                                  << ", SSRC=" << webrtc::ParseRtpSsrc(payload)
                                  << ", previous failure count: "
                                  << decryptionFailureCount;
            }
            ++decryptionFailureCount;
            return;
        }

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