//
// Created by Lauren on 04/06/26.
//

#include <wrtc/interfaces/raw_tcp_socket.hpp>

#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>
#include <rtc_base/network/sent_packet.h>

namespace webrtc {
    static constexpr size_t kMaxPacketSize = 64 * 1024;

    static constexpr size_t kBufSize = kMaxPacketSize + 4;

    RawTcpSocket::RawTcpSocket(std::unique_ptr<Socket> socket): AsyncTCPSocketBase(std::move(socket), kBufSize) {}

    int RawTcpSocket::Send(const void* pv, const size_t cb, const AsyncSocketPacketOptions& options) {
        if (cb > kBufSize) {
            SetError(EMSGSIZE);
            return -1;
        }
        if (!IsOutBufferEmpty())
            return static_cast<int>(cb);

        if (!did_send_mt_proto_prologue_) {
            did_send_mt_proto_prologue_ = true;
            constexpr uint32_t kPrologue = 0xeeeeeeee;
            AppendToOutBuffer(&kPrologue, 4);
        }

        const auto pkt_len = static_cast<uint32_t>(cb);
        AppendToOutBuffer(&pkt_len, 4);
        AppendToOutBuffer(pv, cb);

        if (const auto res = FlushOutBuffer(); res <= 0) {
            ClearOutBuffer();
            return res;
        }
        SentPacketInfo sent_packet_info(options.packet_id, TimeMillis(), options.info_signaled_after_sent);
        CopySocketInformationToPacketInfo(cb, *this, &sent_packet_info.info);
        NotifySentPacket(this, sent_packet_info);
        return static_cast<int>(cb);
    }

    size_t RawTcpSocket::ProcessInput(const bytes::const_span data) {
        const SocketAddress remote_addr(GetRemoteAddress());
        size_t processed_bytes = 0;
        while (true) {
            const size_t bytes_left = data.size() - processed_bytes;
            if (bytes_left < 4) {
                return processed_bytes;
            }

            const uint32_t pkt_len = GetLE32(data.subspan(processed_bytes, 4));
            if (bytes_left < 4 + pkt_len) {
                return processed_bytes;
            }

            const ReceivedIpPacket received_packet(
                data.subspan(processed_bytes + 4, pkt_len),
                remote_addr,
                Timestamp::Micros(TimeMicros())
            );
            NotifyPacketReceived(received_packet);
            processed_bytes += 4 + pkt_len;
        }
    }

    std::unique_ptr<Socket> RawTcpSocket::connect_socket(Socket* socket, const SocketAddress& bind_address, const SocketAddress& remote_address) {
        std::unique_ptr<Socket> owned_socket(socket);
        if (socket->Bind(bind_address) < 0) {
            RTC_LOG(LS_ERROR) << "Bind() failed with error " << socket->GetError();
            return nullptr;
        }
        if (socket->Connect(remote_address) < 0) {
            RTC_LOG(LS_ERROR) << "Connect() failed with error " << socket->GetError();
            return nullptr;
        }
        return std::move(owned_socket);
    }

    std::unique_ptr<RawTcpSocket> RawTcpSocket::create(
        Socket* socket,
        const SocketAddress& bind_address,
        const SocketAddress& remote_address
    ) {
        return std::make_unique<RawTcpSocket>(connect_socket(socket, bind_address, remote_address));
    }
} // webrtc
