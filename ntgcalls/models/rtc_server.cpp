//
// Created by Laky64 on 13/03/2024.
//

#include "rtc_server.hpp"

namespace wrtc {
    RTCServer::RTCServer(
        const uint64_t id,
        std::string ipv4,
        std::string ipv6,
        const uint16_t port,
        const std::optional<std::string>& username,
        const std::optional<std::string>& password,
        const bool turn,
        const bool stun,
        const bool tcp,
        const std::optional<BYTES(bytes::binary)>& peerTag
    ) {
        this->id = id;
        this->ipv4 = std::move(ipv4);
        this->ipv6 = std::move(ipv6);
        this->port = port;
        this->username = username;
        this->password = password;
        this->turn = turn;
        this->stun = stun;
        this->tcp = tcp;
        this->peerTag = CPP_BYTES(peerTag, bytes::binary);
    }

    webrtc::PeerConnectionInterface::IceServers RTCServer::toIceServers(const std::vector<RTCServer>& servers) {
        webrtc::PeerConnectionInterface::IceServers iceServers;
        for (const auto& server: servers) {
            if (server.peerTag) {
                if (server.tcp) {
                    continue;
                }
                const auto hex = [](const bytes::binary& value) {
                    const auto digit = [](const unsigned char c) {
                        return static_cast<char>(c < 10 ? '0' + c : 'a' + c - 10);
                    };
                    auto result = std::string();
                    result.reserve(value.size() * 2);
                    for (const auto ch : value) {
                        result += digit(static_cast<unsigned char>(ch) / 16);
                        result += digit(static_cast<unsigned char>(ch) % 16);
                    }
                    return result;
                };
                const auto pushPhone = [&](const std::string &host) {
                    if (host.empty()) {
                        return;
                    }
                    const rtc::SocketAddress address(host, server.port);
                    if (!address.IsComplete()) {
                        RTC_LOG(LS_ERROR) << "Invalid ICE server host: " << host;
                        return;
                    }
                    webrtc::PeerConnectionInterface::IceServer iceServer;
                    iceServer.uri = "turn:" + address.HostAsURIString() + ":" + std::to_string(server.port);
                    iceServer.username = "reflector";
                    iceServer.password = hex(server.peerTag.value());
                    iceServers.push_back(iceServer);
                    RTC_LOG(LS_INFO) << "PHONE server: " << iceServer.uri << " username: " << iceServer.username << " password: " << iceServer.password;
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
                            RTC_LOG(LS_ERROR) << "Invalid ICE server host: " << host;
                            return;
                        }
                        webrtc::PeerConnectionInterface::IceServer iceServer;
                        iceServer.uri = "stun:" + address.HostAsURIString() + ":" + std::to_string(server.port);
                        iceServers.push_back(iceServer);
                        RTC_LOG(LS_INFO) << "STUN server: " << iceServer.uri;
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
                            RTC_LOG(LS_ERROR) << "Invalid ICE server host: " << host;
                            return;
                        }
                        webrtc::PeerConnectionInterface::IceServer iceServer;
                        iceServer.uri = "turn:" + address.HostAsURIString() + ":" + std::to_string(server.port);
                        iceServer.username = server.username.value();
                        iceServer.password = server.password.value();
                        iceServers.push_back(iceServer);
                        RTC_LOG(LS_INFO) << "TURN server: " << iceServer.uri << " username: " << iceServer.username;
                    };
                    pushTurn(server.ipv4);
                    pushTurn(server.ipv6);
                }
            }

        }
        return iceServers;
    }
} // wrtc