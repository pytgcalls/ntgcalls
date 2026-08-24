//
// Created by Lauren on 29/03/24.
//

#pragma once

#include <p2p/client/relay_port_factory_interface.h>
#include <wrtc/models/rtc_server.hpp>

namespace wrtc::interfaces {

    class ReflectorRelayPortFactory final: public webrtc::RelayPortFactoryInterface {
        std::vector<models::RTCServer> servers_;
        bool standalone_reflector_mode_;
        uint32_t standalone_reflector_role_id_;
        webrtc::SocketFactory* underlying_socket_factory_;

    public:
        explicit ReflectorRelayPortFactory(
            const std::vector<models::RTCServer>& servers,
            bool standalone_reflector_mode,
            uint32_t standalone_reflector_role_id,
            webrtc::SocketFactory* underlying_socket_factory
        );

        ~ReflectorRelayPortFactory() override = default;

        std::unique_ptr<webrtc::Port> Create(const webrtc::CreateRelayPortArgs& args, webrtc::AsyncPacketSocket* udp_socket) override;

        std::unique_ptr<webrtc::Port> Create(const webrtc::CreateRelayPortArgs& args, int min_port, int max_port) override;
    };

} // wrtc::interfaces
