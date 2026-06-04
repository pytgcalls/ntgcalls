//
// Created by laky64 on 04/06/26.
//

#include <wrtc/interfaces/raw_tcp_socket.hpp>

#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>
#include <rtc_base/network/sent_packet.h>

namespace webrtc {
    static constexpr size_t kMaxPacketSize = 64 * 1024;

    static constexpr size_t kBufSize = kMaxPacketSize + 4;

    RawTcpSocket::RawTcpSocket(std::unique_ptr<Socket> socket) : AsyncTCPSocketBase(std::move(socket), kBufSize) {}

    int RawTcpSocket::Send(const void *pv, const size_t cb, const AsyncSocketPacketOptions &options) {
        if (cb > kBufSize) {
            SetError(EMSGSIZE);
            return -1;
        }
        if (!IsOutBufferEmpty())
            return static_cast<int>(cb);

        if (!didSendMTProtoPrologue) {
            didSendMTProtoPrologue = true;
            constexpr uint32_t prologue = 0xeeeeeeee;
            AppendToOutBuffer(&prologue, 4);
        }

        const auto pktLen = static_cast<uint32_t>(cb);
        AppendToOutBuffer(&pktLen, 4);
        AppendToOutBuffer(pv, cb);

        if (const auto res = FlushOutBuffer(); res <= 0) {
            ClearOutBuffer();
            return res;
        }
        SentPacketInfo sentPacketInfo(options.packet_id, TimeMillis(), options.info_signaled_after_sent);
        CopySocketInformationToPacketInfo(cb, *this, &sentPacketInfo.info);
        NotifySentPacket(this, sentPacketInfo);
        return static_cast<int>(cb);
    }

    size_t RawTcpSocket::ProcessInput(const std::span<const uint8_t> data) {
        const SocketAddress remoteAddr(GetRemoteAddress());
        size_t processed_bytes = 0;
        while (true) {
            const size_t bytesLeft = data.size() - processed_bytes;
            if (bytesLeft < 4) {
                return processed_bytes;
            }

            const uint32_t pktLen = GetLE32(data.subspan(processed_bytes, 4));
            if (bytesLeft < 4 + pktLen) {
                return processed_bytes;
            }

            ReceivedIpPacket receivedPacket(
                data.subspan(processed_bytes + 4, pktLen),
                remoteAddr,
                Timestamp::Micros(TimeMicros())
            );
            NotifyPacketReceived(receivedPacket);
            processed_bytes += 4 + pktLen;
        }
    }

    std::unique_ptr<Socket> RawTcpSocket::ConnectSocket(Socket *socket, const SocketAddress &bind_address,
        const SocketAddress &remote_address) {
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

    std::unique_ptr<RawTcpSocket> RawTcpSocket::Create(
        Socket* socket,
        const SocketAddress& bindAddress,
        const SocketAddress& remoteAddress
    ) {
        return std::make_unique<RawTcpSocket>(ConnectSocket(socket, bindAddress, remoteAddress));
    }
} // webrtc