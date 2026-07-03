//
// Created by Lauren on 29/03/24.
//
#include <p2p/base/turn_port.h>
#include <wrtc/interfaces/reflector_port.hpp>
#include <wrtc/interfaces/reflector_relay_port_factory.hpp>

namespace wrtc::interfaces {
    ReflectorRelayPortFactory::ReflectorRelayPortFactory(const std::vector<models::RTCServer>& servers,
        const bool standalone_reflector_mode,
        const uint32_t standalone_reflector_role_id,
        webrtc::SocketFactory* underlying_socket_factory
    ):
    servers_(servers),
    standalone_reflector_mode_(standalone_reflector_mode),
    standalone_reflector_role_id_(standalone_reflector_role_id),
    underlying_socket_factory_(underlying_socket_factory) {}

    std::unique_ptr<webrtc::Port> ReflectorRelayPortFactory::Create(const webrtc::CreateRelayPortArgs& args, webrtc::AsyncPacketSocket* udp_socket) {
        if (args.config->credentials.username == "reflector") {
            uint8_t found_id = 0;
            for (const auto & [id, host, port, login, password, isTurn, isTcp] : servers_) {
                if (const webrtc::SocketAddress server_address(host, port); args.server_address->address == server_address) {
                    found_id = id;
                    break;
                }
            }
            if (found_id == 0) {
                return nullptr;
            }
            auto port = ReflectorPort::create(
                args,
                underlying_socket_factory_,
                udp_socket,
                found_id,
                args.relative_priority,
                standalone_reflector_mode_,
                standalone_reflector_role_id_
            );
            if (!port) {
                return nullptr;
            }
            return port;
        }
        auto port = webrtc::TurnPort::Create(args, udp_socket);
        if (!port) {
            return nullptr;
        }
        port->SetTlsCertPolicy(args.config->tls_cert_policy);
        port->SetTurnLoggingId(args.config->turn_logging_id);
        return port;
    }

    std::unique_ptr<webrtc::Port> ReflectorRelayPortFactory::Create(const webrtc::CreateRelayPortArgs& args, const int min_port, const int max_port) {
        if (args.config->credentials.username == "reflector") {
            uint8_t found_id = 0;
            for (const auto & [id, host, port, login, password, isTurn, isTcp] : servers_) {
                if (const webrtc::SocketAddress server_address(host, port); args.server_address->address == server_address) {
                    found_id = id;
                    break;
                }
            }
            if (found_id == 0) {
                return nullptr;
            }
            auto port = ReflectorPort::create(
                args,
                underlying_socket_factory_,
                min_port,
                max_port,
                found_id,
                args.relative_priority,
                standalone_reflector_mode_,
                standalone_reflector_role_id_
            );
            if (!port) {
                return nullptr;
            }
            return port;
        }
        auto port = webrtc::TurnPort::Create(args, min_port, max_port);
        if (!port) {
            return nullptr;
        }
        port->SetTlsCertPolicy(args.config->tls_cert_policy);
        port->SetTurnLoggingId(args.config->turn_logging_id);
        return port;
    }
} // wrtc::interfaces