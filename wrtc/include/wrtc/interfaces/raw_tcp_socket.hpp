//
// Created by Lauren on 04/06/26.
//

#pragma once

#include <rtc_base/async_tcp_socket.h>
#include <wrtc/utils/binary.hpp>

namespace webrtc {
    class RawTcpSocket final: public AsyncTCPSocketBase {
        bool did_send_mt_proto_prologue_ = false;

        static std::unique_ptr<Socket> connect_socket(
            Socket* socket,
            const SocketAddress& bind_address,
            const SocketAddress& remote_address
        );

    public:
        static std::unique_ptr<RawTcpSocket> create(Socket* socket, const SocketAddress& bind_address, const SocketAddress& remote_address);

        explicit RawTcpSocket(std::unique_ptr<Socket> socket);

        int Send(const void* pv, size_t cb, const AsyncSocketPacketOptions& options) override;

        size_t ProcessInput(bytes::const_span data) override;
    };
} // webrtc
