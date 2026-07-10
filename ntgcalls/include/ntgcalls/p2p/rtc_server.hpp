//
// Created by Lauren on 13/03/24.
//

#pragma once
#include <string>
#include <api/peer_connection_interface.h>
#include <wrtc/models/rtc_server.hpp>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::p2p {
    class RTCServer {
        static std::set<uint64_t> collect_endpoint_ids(const std::vector<RTCServer>& servers);

    public:
        uint64_t id;
        std::string ipv4, ipv6;
        std::optional<std::string> username, password;
        uint16_t port;
        bool turn, stun, tcp;
        std::optional<bytes::binary> peer_tag;

        RTCServer(
            uint64_t id,
            std::string ipv4,
            std::string ipv6,
            uint16_t port,
            const std::optional<std::string>& username,
            const std::optional<std::string>& password,
            bool turn,
            bool stun,
            bool tcp,
            const std::optional<bytes::binary>& peer_tag
        );

        static std::vector<wrtc::models::RTCServer> to_rtc_servers(const std::vector<RTCServer>& servers);

        static webrtc::PeerConnectionInterface::IceServers to_ice_servers(const std::vector<RTCServer>& servers);
    };
} // ntgcalls::p2p
