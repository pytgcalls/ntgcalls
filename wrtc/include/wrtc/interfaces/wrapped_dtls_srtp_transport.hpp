//
// Created by Lauren on 06/10/24.
//

#pragma once
#include <pc/dtls_srtp_transport.h>
#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc::interfaces {

    class WrappedDtlsSrtpTransport final : public webrtc::DtlsSrtpTransport {
        utils::synchronized_callback<void(webrtc::RtpPacketReceived)> rtp_packet_callback_;
        int decryption_failure_count_ = 0;

        void OnRtpPacketReceived(const webrtc::ReceivedIpPacket& packet) override;

    public:
        WrappedDtlsSrtpTransport(
            bool rtcp_mux_enabled,
            const webrtc::FieldTrialsView& field_trials,
            const std::function<void(webrtc::RtpPacketReceived)>& callback
        );

        ~WrappedDtlsSrtpTransport() override;
    };

} // wrtc::interfaces
