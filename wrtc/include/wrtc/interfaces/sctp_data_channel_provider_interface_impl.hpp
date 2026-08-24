//
// Created by Lauren on 29/03/24.
//

#pragma once
#include <api/data_channel_interface.h>
#include <media/sctp/sctp_transport_factory.h>
#include <pc/sctp_data_channel.h>
#include <rtc_base/weak_ptr.h>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc::interfaces {

    class SctpDataChannelProviderInterfaceImpl final: public webrtc::SctpDataChannelControllerInterface, public webrtc::DataChannelObserver, public webrtc::DataChannelSink {
        webrtc::WeakPtrFactory<SctpDataChannelProviderInterfaceImpl> weak_factory_;
        std::unique_ptr<webrtc::SctpTransportFactory> sctp_transport_factory_;
        std::unique_ptr<webrtc::SctpTransportInternal> sctp_transport_;
        webrtc::scoped_refptr<webrtc::SctpDataChannel> data_channel_;
        std::vector<bytes::binary> pending_messages_;
        webrtc::Thread* network_thread_;
        bool is_open_ = false;
        bool is_sctp_transport_started_ = false;

        utils::synchronized_callback<void(bool)> on_state_changed_callback_;
        utils::synchronized_callback<void(bytes::binary)> on_message_received_callback_;

    public:
        SctpDataChannelProviderInterfaceImpl(
            const webrtc::Environment& env,
            webrtc::DtlsTransportInternal* transport_channel,
            bool is_outgoing,
            webrtc::Thread* network_thread
        );

        ~SctpDataChannelProviderInterfaceImpl() override;

        bool IsOkToCallOnTheNetworkThread() override;

        void OnDataReceived(int channel_id, webrtc::DataMessageType type, const webrtc::CopyOnWriteBuffer& buffer) override;

        void OnReadyToSend() override;

        void OnStateChange() override;

        void OnMessage(const webrtc::DataBuffer& buffer) override;

        webrtc::RTCError SendData(webrtc::StreamId sid, const webrtc::SendDataParams& params, const webrtc::CopyOnWriteBuffer& payload) override;

        webrtc::RTCError AddSctpDataStream(webrtc::StreamId sid, webrtc::PriorityValue priority) override;

        void RemoveSctpDataStream(webrtc::StreamId sid) override;

        void update_is_connected(bool is_connected);

        void send_data_channel_message(const bytes::binary& data);

        void OnTransportClosed(webrtc::RTCError) override;

        void on_state_changed(const std::function<void(bool)>& callback);

        void on_message_received(const std::function<void(const bytes::binary&)>& callback);

        // Unused
        void OnChannelClosing(int channel_id) override {}
        void OnChannelClosed(int channel_id) override {}
        void OnChannelStateChanged(webrtc::SctpDataChannel* data_channel, webrtc::DataChannelInterface::DataState state) override {}
        void OnBufferedAmountLow(int channel_id) override {}
        size_t buffered_amount(webrtc::StreamId sid) const override {
            return 0;
        }
        size_t buffered_amount_low_threshold(webrtc::StreamId sid) const override {
            return 0;
        }
        void SetBufferedAmountLowThreshold(webrtc::StreamId sid, size_t bytes) override {}
        void OnTransportConnected() override {}
        void OnMaxMessageSize(int max_message_size) override {}
    };

} // wrtc::interfaces
