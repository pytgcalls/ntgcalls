//
// Created by Laky64 on 06/10/24.
//

#include <modules/rtp_rtcp/source/rtp_packet_received.h>
#include <wrtc/interfaces/wrapped_dtls_srtp_transport.hpp>
#include <modules/rtp_rtcp/source/rtp_util.h>

namespace wrtc {
    template <typename Tag, typename Tag::pfn_t pfn>
    struct tag_bind_pfn {
        friend constexpr Tag::pfn_t pfn_of(Tag) { return pfn; }
    };

    struct tag_SrtpTransport_UnprotectRtp {
        using pfn_t = bool (webrtc::SrtpTransport::*)(webrtc::CopyOnWriteBuffer& buffer);
        friend constexpr pfn_t pfn_of(tag_SrtpTransport_UnprotectRtp);
    };
    template struct tag_bind_pfn<tag_SrtpTransport_UnprotectRtp, &webrtc::SrtpTransport::UnprotectRtp>;

    inline static auto c_pfn_SrtpTransport_UnprotectRtp = pfn_of(tag_SrtpTransport_UnprotectRtp{});

    struct tag_RtpTransport_header_extension_map {
        using pfn_t = webrtc::RtpHeaderExtensionMap webrtc::RtpTransport::*;
        friend constexpr pfn_t pfn_of(tag_RtpTransport_header_extension_map);
    };
    template struct tag_bind_pfn<tag_RtpTransport_header_extension_map, &webrtc::RtpTransport::header_extension_map_>;

    inline static auto c_ptr_RtpTransport_header_extension_map = pfn_of(tag_RtpTransport_header_extension_map{});

    WrappedDtlsSrtpTransport::WrappedDtlsSrtpTransport(
        const bool rtcpMuxEnabled,
        const webrtc::FieldTrialsView& field_trials,
        const std::function<void(webrtc::RtpPacketReceived)>& callback
    ): DtlsSrtpTransport(rtcpMuxEnabled, field_trials) {
        rtpPacketCallback = callback;
    }

    WrappedDtlsSrtpTransport::~WrappedDtlsSrtpTransport() {
        rtpPacketCallback = nullptr;
    }

    void WrappedDtlsSrtpTransport::OnRtpPacketReceived(const webrtc::ReceivedIpPacket& packet) {
        if (!IsSrtpActive()) {
            RTC_LOG(LS_WARNING) << "Inactive SRTP transport received an RTP packet. Drop it.";
            return;
        }

        webrtc::CopyOnWriteBuffer payload(packet.payload());
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

        webrtc::RtpPacketReceived parsedPacket(&(this->*c_ptr_RtpTransport_header_extension_map));
        parsedPacket.set_arrival_time(packet.arrival_time().value_or(webrtc::Timestamp::MinusInfinity()));
        parsedPacket.set_ecn(packet.ecn());

        if (parsedPacket.Parse(payload)) {
            (void) rtpPacketCallback(parsedPacket);
        }

        DemuxPacket(payload, packet.arrival_time().value_or(webrtc::Timestamp::MinusInfinity()), packet.ecn());
    }
} // wrtc