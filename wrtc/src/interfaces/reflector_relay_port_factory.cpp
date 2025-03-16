//
// Created by Laky64 on 29/03/2024.
//
#include <p2p/base/turn_port.h>

#include <wrtc/interfaces/reflector_relay_port_factory.hpp>

#include <wrtc/interfaces/reflector_port.hpp>

namespace wrtc {
    ReflectorRelayPortFactory::ReflectorRelayPortFactory(const std::vector<RTCServer>& servers,
        const bool standaloneReflectorMode,
        const uint32_t standaloneReflectorRoleId):
    servers(servers),
    standaloneReflectorMode(standaloneReflectorMode),
    standaloneReflectorRoleId(standaloneReflectorRoleId) {}

    std::unique_ptr<cricket::Port> ReflectorRelayPortFactory::Create(const cricket::CreateRelayPortArgs& args, rtc::AsyncPacketSocket* udp_socket) {
        if (args.config->credentials.username == "reflector") {
            uint8_t foundId = 0;
            for (const auto & [id, host, port, login, password, isTurn, isTcp] : servers) {
                if (rtc::SocketAddress serverAddress(host, port); args.server_address->address == serverAddress) {
                    foundId = id;
                    break;
                }
            }
            if (foundId == 0) {
                return nullptr;
            }
            auto port = ReflectorPort::Create(args, udp_socket, foundId, args.relative_priority, standaloneReflectorMode, standaloneReflectorRoleId);
            if (!port) {
                return nullptr;
            }
            return port;
        }
        auto port = cricket::TurnPort::Create(args, udp_socket);
        if (!port) {
            return nullptr;
        }
        port->SetTlsCertPolicy(args.config->tls_cert_policy);
        port->SetTurnLoggingId(args.config->turn_logging_id);
        return port;
    }

    std::unique_ptr<cricket::Port> ReflectorRelayPortFactory::Create(const cricket::CreateRelayPortArgs& args, const int min_port, const int max_port) {
        if (args.config->credentials.username == "reflector") {
            uint8_t foundId = 0;
            for (const auto & [id, host, port, login, password, isTurn, isTcp] : servers) {
                if (rtc::SocketAddress serverAddress(host, port); args.server_address->address == serverAddress) {
                    foundId = id;
                    break;
                }
            }
            if (foundId == 0) {
                return nullptr;
            }
            auto port = ReflectorPort::Create(args, min_port, max_port, foundId, args.relative_priority, standaloneReflectorMode, standaloneReflectorRoleId);
            if (!port) {
                return nullptr;
            }
            return port;
        }
        auto port = cricket::TurnPort::Create(args, min_port, max_port);
        if (!port) {
            return nullptr;
        }
        port->SetTlsCertPolicy(args.config->tls_cert_policy);
        port->SetTurnLoggingId(args.config->turn_logging_id);
        return port;
    }
} // wrtc