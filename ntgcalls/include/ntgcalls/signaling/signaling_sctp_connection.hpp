//
// Created by Lauren on 16/03/24.
//

#pragma once
#include <media/sctp/sctp_transport_factory.h>
#include <ntgcalls/signaling/signaling_interface.hpp>
#include <ntgcalls/signaling/signaling_packet_transport.hpp>

namespace ntgcalls::signaling {

    class SignalingSctpConnection final : public SignalingInterface, public webrtc::DataChannelSink {
        std::unique_ptr<webrtc::SctpTransportFactory> sctp_transport_factory_;
        std::unique_ptr<SignalingPacketTransport> packet_transport_;
        std::unique_ptr<webrtc::SctpTransportInternal> sctp_transport_;
        std::vector<bytes::binary> pending_data_;
        bool allow_compression_ = false;
        bool is_ready_to_send_ = false;

    protected:
        [[nodiscard]] bool supports_compression() const override;

    public:
        SignalingSctpConnection(
            wrtc::utils::SafeThread& network_thread,
            wrtc::utils::SafeThread& signaling_thread,
            const webrtc::Environment& env,
            const crypto::EncryptionKey &key,
            const DataEmitter& on_emit_data,
            const DataReceiver& on_signal_data,
            bool allow_compression
        );

        void close() override;

        void receive(const bytes::binary& data) override;

        void send(const bytes::binary& data) override;

        void OnReadyToSend() override;

        void OnDataReceived(int channel_id, webrtc::DataMessageType type, const webrtc::CopyOnWriteBuffer& buffer) override;

        void OnTransportClosed(webrtc::RTCError error) override;

        // Unused
        void OnChannelClosing(int channel_id) override{}
        void OnChannelClosed(int channel_id) override{}
        void OnBufferedAmountLow(int channel_id) override{}
        void OnTransportConnected() override{}
        void OnMaxMessageSize(int max_message_size) override{}
    };

} // ntgcalls::signaling
