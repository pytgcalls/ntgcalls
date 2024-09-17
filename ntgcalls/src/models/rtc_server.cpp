//
// Created by Laky64 on 13/03/2024.
//

#include <ntgcalls/models/rtc_server.hpp>

namespace ntgcalls {
    std::set<int64_t> RTCServer::collectEndpointIds(const std::vector<RTCServer>& servers) {
        std::set<int64_t> result;
        for (const auto &server: servers) {
            if (server.peerTag) {
                result.emplace(server.id);
            }
        }
        return result;
    }

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

    std::vector<wrtc::RTCServer> RTCServer::toRtcServers(const std::vector<RTCServer>& servers) {
        const auto ids = collectEndpointIds(servers);
        std::vector<wrtc::RTCServer> wrtcServers;
        for (const auto& server: servers) {
            if (server.peerTag) {
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
                const auto i = ids.find(server.id);
		        const auto id = static_cast<uint8_t>(std::distance(ids.begin(), i) + 1);
                const auto pushPhone = [&](const std::string &host) {
                    wrtc::RTCServer rtcServer;
                    rtcServer.id = id;
                    rtcServer.host = host;
                    rtcServer.port = server.port;
                    rtcServer.login = "reflector";
                    rtcServer.password = hex(server.peerTag.value());
                    rtcServer.isTurn = true;
                    rtcServer.isTcp = server.tcp;
                    wrtcServers.push_back(rtcServer);
                    RTC_LOG(LS_INFO) << "PHONE server: turn:" << rtcServer.host << ":" << rtcServer.port << " username: " << rtcServer.login << " password: " << rtcServer.password;
                };
                pushPhone(server.ipv4);
                pushPhone(server.ipv6);
            } else {
                if (server.stun) {
                    const auto pushStun = [&](const std::string &host) {
                        wrtc::RTCServer rtcServer;
                        rtcServer.host = host;
                        rtcServer.port = server.port;
                        wrtcServers.push_back(rtcServer);
                        RTC_LOG(LS_INFO) << "STUN server: stun:" << rtcServer.host << ":" << rtcServer.port;
                    };
                    pushStun(server.ipv4);
                    pushStun(server.ipv6);
                }
                if (server.turn && server.username && server.password) {
                    const auto pushTurn = [&](const std::string &host) {
                        wrtc::RTCServer rtcServer;
                        rtcServer.host = host;
                        rtcServer.port = server.port;
                        rtcServer.login = *server.username;
                        rtcServer.password = *server.password;
                        rtcServer.isTurn = true;
                        wrtcServers.push_back(rtcServer);
                        RTC_LOG(LS_INFO) << "TURN server: turn:" << rtcServer.host << ":" << rtcServer.port << " username: " << rtcServer.login << " password: " << rtcServer.password;
                    };
                    pushTurn(server.ipv4);
                    pushTurn(server.ipv6);
                }
            }
        }
        return wrtcServers;
    }

    webrtc::PeerConnectionInterface::IceServers RTCServer::toIceServers(const std::vector<RTCServer>& servers) {
        webrtc::PeerConnectionInterface::IceServers iceServers;
        for (std::vector<wrtc::RTCServer> wrtcServers = toRtcServers(servers); const auto & [id, host, port, login, password, isTurn, isTcp] : wrtcServers) {
            if (isTcp) {
                continue;
            }
            rtc::SocketAddress address(host, port);
            if (!address.IsComplete()) {
                RTC_LOG(LS_ERROR) << "Invalid ICE server host: " << host;
                continue;
            }
            webrtc::PeerConnectionInterface::IceServer iceServer;
            if (isTurn) {
                iceServer.urls.push_back("turn:" + address.HostAsURIString() + ":" + std::to_string(port));
                iceServer.username = login;
                iceServer.password = password;
            } else {
                iceServer.urls.push_back("stun:" + address.HostAsURIString() + ":" + std::to_string(port));
            }
            iceServers.push_back(iceServer);
        }
        return iceServers;
    }
} // wrtc