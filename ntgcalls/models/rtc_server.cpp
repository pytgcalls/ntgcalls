//
// Created by iraci on 13/03/2024.
//

#include "rtc_server.hpp"

#include "wrtc/utils/binary.hpp"

namespace ntgcalls {
    RTCServer::RTCServer(
        std::string ipv4,
        std::string ipv6,
        const uint16_t port,
        const std::optional<std::string>& username,
        const std::optional<std::string>& password,
        const bool turn,
        const bool stun,
        const bool tcp,
        const bytes::binary& peerTag) {
        this->ipv4 = std::move(ipv4);
        this->ipv6 = std::move(ipv6);
        this->port = port;
        this->username = username;
        this->password = password;
        this->turn = turn;
        this->stun = stun;
        this->tcp = tcp;
        this->peerTag = peerTag;
    }

    webrtc::PeerConnectionInterface::IceServers RTCServer::toIceServers(const std::vector<RTCServer>& servers) {
        webrtc::PeerConnectionInterface::IceServers iceServers;
        for (const auto& server: servers) {
            if (server.peerTag) {
                if (server.tcp) {
                    continue;
                }
                const auto hex = [](const unsigned char* value, const size_t size) {
                    const auto digit = [](const unsigned char c) {
                        return static_cast<char>(c < 10 ? '0' + c : 'a' + c - 10);
                    };
                    auto result = std::string();
                    result.reserve(size * 2);
                    for (size_t i = 0; i < size; ++i) {
                        result += digit(static_cast<unsigned char>(value[i]) / 16);
                        result += digit(static_cast<unsigned char>(value[i]) % 16);
                    }
                    return result;
                };
                const auto pushPhone = [&](const std::string &host) {
                    if (host.empty()) {
                        return;
                    }
                    const rtc::SocketAddress address(host, server.port);
                    if (!address.IsComplete()) {
                        return;
                    }
                    webrtc::PeerConnectionInterface::IceServer iceServer;
                    iceServer.uri = "turn:" + address.HostAsURIString() + ":" + std::to_string(server.port);
                    iceServer.username = "reflector";
                    iceServer.password = hex(server.peerTag, server.peerTag.size());
                    iceServers.push_back(iceServer);
                };
                pushPhone(server.ipv4);
                pushPhone(server.ipv6);
            } else {
                if (server.stun) {
                    const auto pushStun = [&](const std::string &host) {
                        if (host.empty()) {
                            return;
                        }
                        const rtc::SocketAddress address(host, server.port);
                        if (!address.IsComplete()) {
                            return;
                        }
                        webrtc::PeerConnectionInterface::IceServer iceServer;
                        iceServer.uri = "stun:" + address.HostAsURIString() + ":" + std::to_string(server.port);
                        iceServers.push_back(iceServer);
                    };
                    pushStun(server.ipv4);
                    pushStun(server.ipv6);
                }
                if (server.turn && server.username && server.password) {
                    const auto pushTurn = [&](const std::string &host) {
                        if (host.empty()) {
                            return;
                        }
                        const rtc::SocketAddress address(host, server.port);
                        if (!address.IsComplete()) {
                            return;
                        }
                        webrtc::PeerConnectionInterface::IceServer iceServer;
                        iceServer.uri = "turn:" + address.HostAsURIString() + ":" + std::to_string(server.port);
                        iceServer.username = server.username.value();
                        iceServer.password = server.password.value();
                        iceServers.push_back(iceServer);
                    };
                    pushTurn(server.ipv4);
                    pushTurn(server.ipv6);
                }
            }

        }
        return iceServers;
    }
} // ntgcalls