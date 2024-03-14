//
// Created by iraci on 14/03/2024.
//

#pragma once
#include <memory>

#include "signaling.hpp"
#include "signaling_packet_transport.hpp"
#include "media/sctp/sctp_transport_factory.h"
#include "wrtc/utils/binary.hpp"


namespace ntgcalls {
    class SignalingConnection final : public webrtc::DataChannelSink {
    public:
        enum class ProtocolVersion{
            V1,
            V2
        };

        explicit SignalingConnection(
            rtc::Thread* network_thread,
            ProtocolVersion version,
            bool isOutGoing,
            const bytes::binary& key,
            const std::function<void(const bytes::binary&)>& onEmitData,
            const std::function<void(const bytes::binary&)>& onSignalData
        );

        ~SignalingConnection() override;

        void receive(const bytes::binary &data) const;

        void send(const bytes::binary &data);

        void OnReadyToSend() override;

        void OnDataReceived(int channel_id, webrtc::DataMessageType type, const rtc::CopyOnWriteBuffer& buffer) override;

        // Unused
        void OnChannelClosing(int channel_id) override{}
        void OnChannelClosed(int channel_id) override{}

    private:
        std::shared_ptr<Signaling> signaling;
        std::unique_ptr<cricket::SctpTransportFactory> sctpTransportFactory;
        std::unique_ptr<SignalingPacketTransport> packetTransport;
        std::unique_ptr<cricket::SctpTransportInternal> sctpTransport;
        rtc::Thread* network_thread;
        std::function<void(const bytes::binary&)> onEmitData, onSignalData;
        bool isReadyToSend = false;
        std::vector<bytes::binary> pendingData;
        std::mutex mutex;
        ProtocolVersion version;
    };

} // ntgcalls
