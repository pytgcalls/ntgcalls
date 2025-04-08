//
// Created by Laky64 on 14/03/2024.
//

#pragma once

#include <pc/dtls_transport.h>
#include <rtc_base/rtc_certificate.h>
#include <wrtc/utils/binary.hpp>

namespace signaling {

    class SignalingPacketTransport final : public cricket::DtlsTransportInternal {
        std::function<void(const bytes::binary&)> emitData;
        std::string transportName;
    public:
        explicit SignalingPacketTransport(const std::function<void(const bytes::binary&)>& emitData): emitData(emitData), transportName("signaling") {}

        void receiveData(const bytes::binary& data);

        [[nodiscard]] const std::string& transport_name() const override;

        [[nodiscard]] bool writable() const override;

        [[nodiscard]] bool receiving() const override;

        int SendPacket(const char* data, size_t len, const rtc::PacketOptions& options, int flags) override;

        int SetOption(rtc::Socket::Option opt, int value) override;

        bool GetOption(rtc::Socket::Option opt, int* value) override;

        int GetError() override;

        [[nodiscard]] std::optional<rtc::NetworkRoute> network_route() const override;

        webrtc::DtlsTransportState dtls_state() const override;

        int component() const override;

        bool IsDtlsActive() const override;

        bool GetDtlsRole(rtc::SSLRole* role) const override;

        bool SetDtlsRole(rtc::SSLRole role) override;

        bool GetSslVersionBytes(int* version) const override;

        bool GetSrtpCryptoSuite(int* cipher) const override;

        bool GetSslCipherSuite(int* cipher) const override;

        std::optional<absl::string_view> GetTlsCipherSuiteName() const override;

        uint16_t GetSslPeerSignatureAlgorithm() const override;

        rtc::scoped_refptr<rtc::RTCCertificate> GetLocalCertificate() const override;

        bool SetLocalCertificate(const rtc::scoped_refptr<rtc::RTCCertificate>& certificate) override;

        std::unique_ptr<rtc::SSLCertChain> GetRemoteSSLCertChain() const override;

        bool ExportSrtpKeyingMaterial(rtc::ZeroOnFreeBuffer<uint8_t>& keying_material) override;

        bool SetRemoteFingerprint(absl::string_view digest_alg, const uint8_t* digest, size_t digest_len) override;

        webrtc::RTCError SetRemoteParameters(absl::string_view digest_alg, const uint8_t* digest, size_t digest_len, std::optional<rtc::SSLRole> role) override;

        cricket::IceTransportInternal* ice_transport() override;
    };

} // signaling
