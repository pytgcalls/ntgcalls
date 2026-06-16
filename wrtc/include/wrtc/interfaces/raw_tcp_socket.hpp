//
// Created by laky64 on 04/06/26.
//

#pragma once

#include <rtc_base/async_tcp_socket.h>

namespace webrtc {
    class RawTcpSocket final : public AsyncTCPSocketBase {
        bool didSendMTProtoPrologue = false;

        static std::unique_ptr<Socket> ConnectSocket(
            Socket* socket,
            const SocketAddress& bind_address,
            const SocketAddress& remote_address
        );

    public:
        static std::unique_ptr<RawTcpSocket> Create(Socket* socket, const SocketAddress& bindAddress, const SocketAddress& remoteAddress);

        explicit RawTcpSocket(std::unique_ptr<Socket> socket);

        int Send(const void* pv, size_t cb, const AsyncSocketPacketOptions& options) override;

        size_t ProcessInput(std::span<const uint8_t> data) override;
    };
} // webrtc
