//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include <rtc_base/weak_ptr.h>
#include <api/data_channel_interface.h>
#include <pc/sctp_data_channel.h>
#include <media/sctp/sctp_transport_factory.h>

#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/syncronized_callback.hpp>

namespace wrtc {

    class SctpDataChannelProviderInterfaceImpl final : public sigslot::has_slots<>, public webrtc::SctpDataChannelControllerInterface, public webrtc::DataChannelObserver, public webrtc::DataChannelSink {
        rtc::WeakPtrFactory<SctpDataChannelProviderInterfaceImpl> weakFactory;
        std::unique_ptr<cricket::SctpTransportFactory> sctpTransportFactory;
        std::unique_ptr<cricket::SctpTransportInternal> sctpTransport;
        rtc::scoped_refptr<webrtc::SctpDataChannel> dataChannel;
        std::vector<bytes::binary> pendingMessages;
        rtc::Thread* networkThread;
        bool isOpen = false;
        bool isSctpTransportStarted = false;

        synchronized_callback<bool> onStateChangedCallback;
        synchronized_callback<bytes::binary> onMessageReceivedCallback;

    public:
        SctpDataChannelProviderInterfaceImpl(
            const webrtc::Environment& env,
            rtc::PacketTransportInternal* transportChannel,
            bool isOutgoing,
            rtc::Thread* networkThread
        );

        ~SctpDataChannelProviderInterfaceImpl() override;

        bool IsOkToCallOnTheNetworkThread() override;

        void OnDataReceived(int channel_id, webrtc::DataMessageType type, const rtc::CopyOnWriteBuffer& buffer) override;

        void OnReadyToSend() override;

        void OnStateChange() override;

        void OnMessage(const webrtc::DataBuffer& buffer) override;

        webrtc::RTCError SendData(webrtc::StreamId sid, const webrtc::SendDataParams& params, const rtc::CopyOnWriteBuffer& payload) override;

        void AddSctpDataStream(webrtc::StreamId sid, webrtc::PriorityValue priority) override;

        void RemoveSctpDataStream(webrtc::StreamId sid) override;

        void updateIsConnected(bool isConnected);

        void sendDataChannelMessage(const bytes::binary& data);

        void OnTransportClosed(webrtc::RTCError) override;

        void onStateChanged(const std::function<void(bool)>& callback);

        void onMessageReceived(const std::function<void(const bytes::binary&)>& callback);

        // Unused
        void OnChannelClosing(int channel_id) override {}
        void OnChannelClosed(int channel_id) override{}
        void OnChannelStateChanged(webrtc::SctpDataChannel* data_channel, webrtc::DataChannelInterface::DataState state) override{}
        void OnBufferedAmountLow(int channel_id) override {}
        size_t buffered_amount(webrtc::StreamId sid) const override { return 0; }
        size_t buffered_amount_low_threshold(webrtc::StreamId sid) const override { return 0;}
        void SetBufferedAmountLowThreshold(webrtc::StreamId sid, size_t bytes) override {}
    };

} // wrtc