//
// Created by Laky64 on 14/03/2024.
//

#include <ntgcalls/signaling/signaling_packet_transport.hpp>

namespace signaling {
    void SignalingPacketTransport::receiveData(const bytes::binary& data) {
        NotifyPacketReceived(
            rtc::ReceivedPacket(
                  rtc::MakeArrayView(data.data(), data.size()),
                rtc::SocketAddress()
            )
        );
    }

    const std::string& SignalingPacketTransport::transport_name() const {
        return transportName;
    }

    bool SignalingPacketTransport::writable() const {
        return true;
    }

    bool SignalingPacketTransport::receiving() const {
        return false;
    }

    int SignalingPacketTransport::SendPacket(const char* data, const size_t len, const rtc::PacketOptions& options, int flags) {
        emitData(bytes::binary(data, data + len));
        rtc::SentPacket sentPacket;
        sentPacket.packet_id = options.packet_id;
        SignalSentPacket.emit(this, sentPacket);
        return static_cast<int>(len);
    }

    int SignalingPacketTransport::SetOption(rtc::Socket::Option opt, int value) {
        return 0;
    }

    bool SignalingPacketTransport::GetOption(rtc::Socket::Option opt, int* value) {
        return false;
    }

    int SignalingPacketTransport::GetError() {
        return 0;
    }

    std::optional<rtc::NetworkRoute> SignalingPacketTransport::network_route() const {
        return std::nullopt;
    }

    webrtc::DtlsTransportState SignalingPacketTransport::dtls_state() const {
        return webrtc::DtlsTransportState::kNew;
    }

    int SignalingPacketTransport::component() const {
        return 0;
    }

    bool SignalingPacketTransport::IsDtlsActive() const {
        return false;
    }

    bool SignalingPacketTransport::GetDtlsRole(rtc::SSLRole* role) const {
        return false;
    }

    bool SignalingPacketTransport::SetDtlsRole(rtc::SSLRole role) {
        return false;
    }

    bool SignalingPacketTransport::GetSslVersionBytes(int* version) const {
        return false;
    }

    bool SignalingPacketTransport::GetSrtpCryptoSuite(int* cipher) const {
        return false;
    }

    bool SignalingPacketTransport::GetSslCipherSuite(int* cipher) const {
        return false;
    }

    std::optional<absl::string_view> SignalingPacketTransport::GetTlsCipherSuiteName() const {
        return std::nullopt;
    }

    uint16_t SignalingPacketTransport::GetSslPeerSignatureAlgorithm() const {
        return 0;
    }

    rtc::scoped_refptr<rtc::RTCCertificate> SignalingPacketTransport::GetLocalCertificate() const {
        return nullptr;
    }

    bool SignalingPacketTransport::SetLocalCertificate(const rtc::scoped_refptr<rtc::RTCCertificate>& certificate) {
        return false;
    }

    std::unique_ptr<rtc::SSLCertChain> SignalingPacketTransport::GetRemoteSSLCertChain() const {
        return nullptr;
    }

    bool SignalingPacketTransport::ExportSrtpKeyingMaterial(rtc::ZeroOnFreeBuffer<uint8_t>& keying_material) {
        return false;
    }

    bool SignalingPacketTransport::SetRemoteFingerprint(absl::string_view digest_alg, const uint8_t* digest, size_t digest_len) {
        return true;
    }

    webrtc::RTCError SignalingPacketTransport::SetRemoteParameters(absl::string_view digest_alg, const uint8_t* digest, size_t digest_len, std::optional<rtc::SSLRole> role) {
        return webrtc::RTCError::OK();
    }

    cricket::IceTransportInternal* SignalingPacketTransport::ice_transport() {
        return nullptr;
    }
} // signaling