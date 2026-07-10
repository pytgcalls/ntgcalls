//
// Created by Lauren on 13/03/24.
//

#include <ntgcalls/p2p/rtc_server.hpp>

namespace ntgcalls::p2p {
    std::set<uint64_t> RTCServer::collect_endpoint_ids(const std::vector<RTCServer>& servers) {
        std::set<uint64_t> result;
        for (const auto &server: servers) {
            if (server.peer_tag) {
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
        const std::optional<bytes::binary>& peer_tag
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
        this->peer_tag = peer_tag;
    }

    std::vector<wrtc::models::RTCServer> RTCServer::to_rtc_servers(const std::vector<RTCServer>& servers) {
        const auto ids = collect_endpoint_ids(servers);
        std::vector<wrtc::models::RTCServer> wrtc_servers;
        for (const auto& server: servers) {
            if (server.peer_tag) {
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
                const auto push_phone = [&](const std::string &host) {
                    wrtc::models::RTCServer rtc_server;
                    rtc_server.id = id;
                    rtc_server.host = host;
                    rtc_server.port = server.port;
                    rtc_server.login = "reflector";
                    rtc_server.password = hex(server.peer_tag.value());
                    rtc_server.is_turn = true;
                    rtc_server.is_tcp = server.tcp;
                    wrtc_servers.push_back(rtc_server);
                    RTC_LOG(LS_VERBOSE) << "PHONE server: turn:" << rtc_server.host << ":" << rtc_server.port << " username: " << rtc_server.login << " password: " << rtc_server.password;
                };
                push_phone(server.ipv4);
                push_phone(server.ipv6);
            } else {
                if (server.stun) {
                    const auto push_stun = [&](const std::string &host) {
                        wrtc::models::RTCServer rtc_server;
                        rtc_server.host = host;
                        rtc_server.port = server.port;
                        wrtc_servers.push_back(rtc_server);
                        RTC_LOG(LS_VERBOSE) << "STUN server: stun:" << rtc_server.host << ":" << rtc_server.port;
                    };
                    push_stun(server.ipv4);
                    push_stun(server.ipv6);
                }
                if (server.turn && server.username && server.password) {
                    const auto push_turn = [&](const std::string &host) {
                        wrtc::models::RTCServer rtc_server;
                        rtc_server.host = host;
                        rtc_server.port = server.port;
                        rtc_server.login = *server.username;
                        rtc_server.password = *server.password;
                        rtc_server.is_turn = true;
                        wrtc_servers.push_back(rtc_server);
                        RTC_LOG(LS_VERBOSE) << "TURN server: turn:" << rtc_server.host << ":" << rtc_server.port << " username: " << rtc_server.login << " password: " << rtc_server.password;
                    };
                    push_turn(server.ipv4);
                    push_turn(server.ipv6);
                }
            }
        }
        return wrtc_servers;
    }

    webrtc::PeerConnectionInterface::IceServers RTCServer::to_ice_servers(const std::vector<RTCServer>& servers) {
        webrtc::PeerConnectionInterface::IceServers ice_servers;
        for (const std::vector<wrtc::models::RTCServer> wrtc_servers = to_rtc_servers(servers); const auto & [id, host, port, login, password, isTurn, isTcp] : wrtc_servers) {
            if (isTcp) {
                continue;
            }
            const webrtc::SocketAddress address(host, port);
            if (!address.IsComplete()) {
                RTC_LOG(LS_ERROR) << "Invalid ICE server host: " << host;
                continue;
            }
            webrtc::PeerConnectionInterface::IceServer ice_server;
            if (isTurn) {
                ice_server.urls.push_back("turn:" + address.HostAsURIString() + ":" + std::to_string(port));
                ice_server.username = login;
                ice_server.password = password;
            } else {
                ice_server.urls.push_back("stun:" + address.HostAsURIString() + ":" + std::to_string(port));
            }
            ice_servers.push_back(ice_server);
        }
        return ice_servers;
    }
} // ntgcalls::p2p