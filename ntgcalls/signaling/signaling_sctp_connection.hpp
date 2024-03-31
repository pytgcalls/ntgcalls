//
// Created by Laky64 on 16/03/2024.
//

#pragma once
#include "signaling_interface.hpp"
#include "signaling_packet_transport.hpp"
#include "media/sctp/sctp_transport_factory.h"

namespace signaling {

    class SignalingSctpConnection final : public sigslot::has_slots<>, public SignalingInterface, public webrtc::DataChannelSink {
        std::unique_ptr<cricket::SctpTransportFactory> sctpTransportFactory;
        std::unique_ptr<SignalingPacketTransport> packetTransport;
        std::unique_ptr<cricket::SctpTransportInternal> sctpTransport;
        std::vector<bytes::binary> pendingData;
        bool allowCompression = false;
        bool isReadyToSend = false;

    public:
        SignalingSctpConnection(
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread,
            const EncryptionKey &key,
            const DataEmitter& onEmitData,
            const DataReceiver& onSignalData,
            bool allowCompression
        );

        ~SignalingSctpConnection() override;

        void receive(const bytes::binary& data) const override;

        void send(const bytes::binary& data) override;

        void OnReadyToSend() override;

        void OnDataReceived(int channel_id, webrtc::DataMessageType type, const rtc::CopyOnWriteBuffer& buffer) override;

        void OnTransportClosed(webrtc::RTCError error) override;

        // Unused
        void OnChannelClosing(int channel_id) override{}
        void OnChannelClosed(int channel_id) override{}

    protected:
        [[nodiscard]] bool supportsCompression() const override;
    };

} // signaling
