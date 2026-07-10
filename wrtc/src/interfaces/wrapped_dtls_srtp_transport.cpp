//
// Created by Lauren on 06/10/24.
//

#include <modules/rtp_rtcp/source/rtp_packet_received.h>
#include <modules/rtp_rtcp/source/rtp_util.h>
#include <wrtc/interfaces/wrapped_dtls_srtp_transport.hpp>

namespace wrtc::interfaces {
    template <typename Tag, typename Tag::pfn_t pfn>
    struct TagBindPfn {
        friend constexpr Tag::pfn_t pfn_of(Tag) { return pfn; }
    };

    struct TagSrtpTransportUnprotectRtp {
        using pfn_t = bool (webrtc::SrtpTransport::*)(webrtc::CopyOnWriteBuffer& buffer);
        friend constexpr pfn_t pfn_of(TagSrtpTransportUnprotectRtp);
    };
    template struct TagBindPfn<TagSrtpTransportUnprotectRtp, &webrtc::SrtpTransport::UnprotectRtp>;

    inline static auto c_pfn_srtp_transport_unprotect_rtp = pfn_of(TagSrtpTransportUnprotectRtp{});

    struct TagRtpTransportHeaderExtensionMap {
        using pfn_t = webrtc::RtpHeaderExtensionMap webrtc::RtpTransport::*;
        friend constexpr pfn_t pfn_of(TagRtpTransportHeaderExtensionMap);
    };
    template struct TagBindPfn<TagRtpTransportHeaderExtensionMap, &webrtc::RtpTransport::header_extension_map_>;

    inline static auto c_ptr_rtp_transport_header_extension_map = pfn_of(TagRtpTransportHeaderExtensionMap{});

    WrappedDtlsSrtpTransport::WrappedDtlsSrtpTransport(
        const bool rtcp_mux_enabled,
        const webrtc::FieldTrialsView& field_trials,
        const std::function<void(webrtc::RtpPacketReceived)>& callback
    ): DtlsSrtpTransport(rtcp_mux_enabled, field_trials) {
        rtp_packet_callback_ = callback;
    }

    WrappedDtlsSrtpTransport::~WrappedDtlsSrtpTransport() {
        rtp_packet_callback_ = nullptr;
    }

    void WrappedDtlsSrtpTransport::OnRtpPacketReceived(const webrtc::ReceivedIpPacket& packet) {
        if (!IsSrtpActive()) {
            RTC_LOG(LS_WARNING) << "Inactive SRTP transport received an RTP packet. Drop it.";
            return;
        }

        webrtc::CopyOnWriteBuffer payload(packet.payload());
        if (!(this->*c_pfn_srtp_transport_unprotect_rtp)(payload)) {
            if (decryption_failure_count_ % 100 == 0) {
                RTC_LOG(LS_ERROR) << "Failed to unprotect RTP packet: size=" << payload.size()
                                  << ", seqnum=" << webrtc::ParseRtpSequenceNumber(payload)
                                  << ", SSRC=" << webrtc::ParseRtpSsrc(payload)
                                  << ", previous failure count: "
                                  << decryption_failure_count_;
            }
            ++decryption_failure_count_;
            return;
        }

        webrtc::RtpPacketReceived parsed_packet(&(this->*c_ptr_rtp_transport_header_extension_map));
        parsed_packet.set_arrival_time(packet.arrival_time().value_or(webrtc::Timestamp::MinusInfinity()));
        parsed_packet.set_ecn(packet.ecn());

        if (parsed_packet.Parse(payload)) {
            (void) rtp_packet_callback_(parsed_packet);
        }

        DemuxPacket(payload, packet.arrival_time().value_or(webrtc::Timestamp::MinusInfinity()), packet.ecn());
    }
} // wrtc::interfaces