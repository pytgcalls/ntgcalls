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
} // signaling