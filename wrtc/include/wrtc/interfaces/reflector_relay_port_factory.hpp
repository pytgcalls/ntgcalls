//
// Created by Laky64 on 29/03/2024.
//

#pragma once

#include <p2p/client/relay_port_factory_interface.h>

#include <wrtc/models/rtc_server.hpp>

namespace wrtc {

    class ReflectorRelayPortFactory final : public cricket::RelayPortFactoryInterface {
        std::vector<RTCServer> servers;
        bool standaloneReflectorMode;
        uint32_t standaloneReflectorRoleId;
    public:
        explicit ReflectorRelayPortFactory(const std::vector<RTCServer>& servers, bool standaloneReflectorMode, uint32_t standaloneReflectorRoleId);

        ~ReflectorRelayPortFactory() override = default;

        std::unique_ptr<cricket::Port> Create(const cricket::CreateRelayPortArgs& args, rtc::AsyncPacketSocket* udp_socket) override;

        std::unique_ptr<cricket::Port> Create(const cricket::CreateRelayPortArgs& args, int min_port, int max_port) override;
    };

} // wrtc