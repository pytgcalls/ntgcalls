//
// Created by Laky64 on 29/03/2024.
//

#include "sctp_data_channel_provider_interface_impl.hpp"

#include <iostream>
#include <p2p/base/dtls_transport.h>
#include <memory>

namespace wrtc {
    SctpDataChannelProviderInterfaceImpl::SctpDataChannelProviderInterfaceImpl(
        const webrtc::Environment& env,
        rtc::PacketTransportInternal* transportChannel,
        const bool isOutgoing,
        rtc::Thread* networkThread,
        rtc::Thread* signalingThread
    ): weakFactory(this), networkThread(networkThread) {
        assert(networkThread->IsCurrent());
        sctpTransportFactory = std::make_unique<cricket::SctpTransportFactory>(networkThread);
        sctpTransport = sctpTransportFactory->CreateSctpTransport(env, transportChannel);
        sctpTransport->SetDataChannelSink(this);

        webrtc::InternalDataChannelInit dataChannelInit;
        dataChannelInit.id = 0;
        dataChannelInit.open_handshake_role = isOutgoing ? webrtc::InternalDataChannelInit::kOpener : webrtc::InternalDataChannelInit::kAcker;
        dataChannel = webrtc::SctpDataChannel::Create(
            weakFactory.GetWeakPtr(),
            "data",
            true,
            dataChannelInit,
            signalingThread,
            networkThread
        );
        if (dataChannel == nullptr) {
            return;
        }
        dataChannel->RegisterObserver(this);
        AddSctpDataStream(webrtc::StreamId(0));
    }

    SctpDataChannelProviderInterfaceImpl::~SctpDataChannelProviderInterfaceImpl() {
        assert(networkThread->IsCurrent());
        weakFactory.InvalidateWeakPtrs();
        dataChannel->UnregisterObserver();
        dataChannel->Close();
        dataChannel = nullptr;
        sctpTransport = nullptr;
        sctpTransportFactory = nullptr;
    }

    bool SctpDataChannelProviderInterfaceImpl::IsOkToCallOnTheNetworkThread() {
        return true;
    }

    void SctpDataChannelProviderInterfaceImpl::OnDataReceived(int channel_id, const webrtc::DataMessageType type, const rtc::CopyOnWriteBuffer& buffer) {
        std::cout << "OnDataReceived" << std::endl;
        assert(networkThread->IsCurrent());
        dataChannel->OnDataReceived(type, buffer);
    }

    void SctpDataChannelProviderInterfaceImpl::OnReadyToSend() {
        std::cout << "OnReadyToSend" << std::endl;
        assert(networkThread->IsCurrent());
        dataChannel->OnTransportReady();
    }

    void SctpDataChannelProviderInterfaceImpl::OnStateChange() {
        std::cout << "OnStateChange" << std::endl;
        assert(networkThread->IsCurrent());
        const auto state = dataChannel->state();
        if (const bool isDataChannelOpen = state == webrtc::DataChannelInterface::DataState::kOpen; isOpen != isDataChannelOpen) {
            isOpen = isDataChannelOpen;
            (void) onStateChangedCallback(isDataChannelOpen);
        }
    }

    void SctpDataChannelProviderInterfaceImpl::OnMessage(const webrtc::DataBuffer& buffer) {
        std::cout << "OnMessage" << std::endl;
        assert(networkThread->IsCurrent());
    }

    webrtc::RTCError SctpDataChannelProviderInterfaceImpl::SendData(const webrtc::StreamId sid, const webrtc::SendDataParams& params, const rtc::CopyOnWriteBuffer& payload) {
        std::cout << "SendData" << std::endl;
        assert(networkThread->IsCurrent());
        return sctpTransport->SendData(sid.stream_id_int(), params, payload);
    }

    void SctpDataChannelProviderInterfaceImpl::AddSctpDataStream(const webrtc::StreamId sid) {
        std::cout << "AddSctpDataStream" << std::endl;
        assert(networkThread->IsCurrent());
        sctpTransport->OpenStream(sid.stream_id_int());
    }

    void SctpDataChannelProviderInterfaceImpl::RemoveSctpDataStream(webrtc::StreamId sid) {
        std::cout << "RemoveSctpDataStream" << std::endl;
        assert(networkThread->IsCurrent());
        networkThread->BlockingCall([this, sid] {
            sctpTransport->ResetStream(sid.stream_id_int());
        });
    }

    void SctpDataChannelProviderInterfaceImpl::updateIsConnected(const bool isConnected) {
        std::cout << "updateIsConnected" << std::endl;
        assert(networkThread->IsCurrent());
        if (isConnected) {
            if (!isSctpTransportStarted) {
                isSctpTransportStarted = true;
                std::cout << "isSctpTransportStarted" << std::endl;
                sctpTransport->Start(5000, 5000, 262144);
            }
        }
    }

    void SctpDataChannelProviderInterfaceImpl::sendDataChannelMessage(const bytes::binary& data) const {
        std::cout << "sendDataChannelMessage" << std::endl;
        if (isOpen) {
            const std::string message = bytes::to_string(data);
            RTC_LOG(LS_INFO) << "Outgoing DataChannel message: " << message;
            const webrtc::DataBuffer buffer(message);
            dataChannel->Send(buffer);
        } else {
            RTC_LOG(LS_INFO) << "Could not send an outgoing DataChannel message: the channel is not open";
        }
    }

    void SctpDataChannelProviderInterfaceImpl::onStateChanged(const std::function<void(bool)>& callback) {
        onStateChangedCallback = callback;
    }
} // wrtc