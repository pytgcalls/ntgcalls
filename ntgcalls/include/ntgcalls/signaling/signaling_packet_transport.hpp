//
// Created by Laky64 on 14/03/2024.
//

#pragma once
#include <p2p/base/packet_transport_internal.h>

#include <wrtc/utils/binary.hpp>

namespace signaling {

    class SignalingPacketTransport final : public rtc::PacketTransportInternal {
        std::function<void(const bytes::binary&)> emitData;
        std::string transportName;
    public:
        explicit SignalingPacketTransport(const std::function<void(const bytes::binary&)>& emitData): emitData(emitData), transportName("signaling") {}

        void receiveData(const bytes::binary& data);

        [[nodiscard]] const std::string& transport_name() const override;

        [[nodiscard]] bool writable() const override;

        [[nodiscard]] bool receiving() const override;

        int SendPacket(const char* data, size_t len, const rtc::PacketOptions& options, int flags) override;

        int SetOption(rtc::Socket::Option opt, int value) override;

        bool GetOption(rtc::Socket::Option opt, int* value) override;

        int GetError() override;

        [[nodiscard]] std::optional<rtc::NetworkRoute> network_route() const override;
    };

} // signaling
