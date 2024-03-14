//
// Created by iraci on 13/03/2024.
//

#pragma once
#include <string>
#include <api/peer_connection_interface.h>

#include "wrtc/utils/binary.hpp"

namespace ntgcalls {
    class RTCServer {
    public:
        std::string ipv4, ipv6;
        std::optional<std::string> username, password;
        uint16_t port;
        bool turn, stun, tcp;
        bytes::binary peerTag;

        RTCServer(
            std::string ipv4,
            std::string ipv6,
            uint16_t port,
            const std::optional<std::string>& username,
            const std::optional<std::string>& password,
            bool turn,
            bool stun,
            bool tcp,
            const bytes::binary& peerTag
        );

        static webrtc::PeerConnectionInterface::IceServers toIceServers(const std::vector<RTCServer>& servers);
    };
} // ntgcalls
