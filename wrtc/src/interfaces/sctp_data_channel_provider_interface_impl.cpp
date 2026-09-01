//
// Created by Lauren on 29/03/24.
//

#include <memory>
#include <optional>
#include <api/task_queue/pending_task_safety_flag.h>
#include <p2p/base/dtls_transport.h>
#include <wrtc/interfaces/sctp_data_channel_provider_interface_impl.hpp>

namespace wrtc::interfaces {
    SctpDataChannelProviderInterfaceImpl::SctpDataChannelProviderInterfaceImpl(
        const webrtc::Environment& env,
        webrtc::DtlsTransportInternal* transport_channel,
        const bool is_outgoing,
        webrtc::Thread* network_thread
    ): weak_factory_(this), network_thread_(network_thread) {
        assert(networkThread->IsCurrent());
        sctp_transport_factory_ = std::make_unique<webrtc::SctpTransportFactory>(network_thread);
        sctp_transport_ = sctp_transport_factory_->CreateSctpTransport(env, transport_channel);
        sctp_transport_->SetDataChannelSink(this);

        webrtc::InternalDataChannelInit data_channel_init;
        data_channel_init.id = 0;
        data_channel_init.open_handshake_role = is_outgoing ? webrtc::InternalDataChannelInit::kOpener : webrtc::InternalDataChannelInit::kAcker;
        data_channel_ = webrtc::SctpDataChannel::Create(
            weak_factory_.GetWeakPtr(),
            "data",
            true,
            data_channel_init,
            std::nullopt,
            webrtc::PendingTaskSafetyFlag::CreateDetached(),
            network_thread,
            network_thread
        );
        if (data_channel_ == nullptr) {
            return;
        }
        data_channel_->RegisterObserver(this);
        AddSctpDataStream(webrtc::StreamId(0), webrtc::PriorityValue(webrtc::Priority::kVeryLow));
    }

    SctpDataChannelProviderInterfaceImpl::~SctpDataChannelProviderInterfaceImpl() {
        assert(networkThread->IsCurrent());
        weak_factory_.InvalidateWeakPtrs();
        on_state_changed_callback_ = nullptr;
        on_message_received_callback_ = nullptr;
        data_channel_->UnregisterObserver();
        data_channel_->Close();
        data_channel_ = nullptr;
        sctp_transport_ = nullptr;
        sctp_transport_factory_ = nullptr;
    }

    bool SctpDataChannelProviderInterfaceImpl::IsOkToCallOnTheNetworkThread() {
        return true;
    }

    void SctpDataChannelProviderInterfaceImpl::OnDataReceived(int channel_id, const webrtc::DataMessageType type, const webrtc::CopyOnWriteBuffer& buffer) {
        assert(networkThread->IsCurrent());
        data_channel_->OnDataReceived(type, buffer);
    }

    void SctpDataChannelProviderInterfaceImpl::OnReadyToSend() {
        assert(networkThread->IsCurrent());
        data_channel_->OnTransportReady();
    }

    void SctpDataChannelProviderInterfaceImpl::OnStateChange() {
        assert(networkThread->IsCurrent());
        const auto state = data_channel_->state();
        if (const bool is_data_channel_open = state == webrtc::DataChannelInterface::DataState::kOpen; is_open_ != is_data_channel_open) {
            is_open_ = is_data_channel_open;
            if (is_open_) {
                for (const auto& message : pending_messages_) {
                    send_data_channel_message(message);
                }
                pending_messages_.clear();
            }
            (void) on_state_changed_callback_(is_data_channel_open);
        }
    }

    void SctpDataChannelProviderInterfaceImpl::OnMessage(const webrtc::DataBuffer& buffer) {
        assert(networkThread->IsCurrent());
        (void) on_message_received_callback_(bytes::binary(buffer.data.data(), buffer.data.data() + buffer.data.size()));
    }

    webrtc::RTCError SctpDataChannelProviderInterfaceImpl::SendData(const webrtc::StreamId sid, const webrtc::SendDataParams& params, const webrtc::CopyOnWriteBuffer& payload) {
        assert(networkThread->IsCurrent());
        return sctp_transport_->SendData(sid.stream_id_int(), params, payload);
    }

    webrtc::RTCError SctpDataChannelProviderInterfaceImpl::AddSctpDataStream(const webrtc::StreamId sid, const webrtc::PriorityValue priority) {
        assert(networkThread->IsCurrent());
        sctp_transport_->OpenStream(sid.stream_id_int(), priority);
        return webrtc::RTCError::OK();
    }

    void SctpDataChannelProviderInterfaceImpl::RemoveSctpDataStream(const webrtc::StreamId sid) {
        assert(networkThread->IsCurrent());
        network_thread_->BlockingCall([this, sid] {
            sctp_transport_->ResetStream(sid.stream_id_int());
        });
    }

    void SctpDataChannelProviderInterfaceImpl::update_is_connected(const bool is_connected) {
        assert(networkThread->IsCurrent());
        if (is_connected) {
            if (!is_sctp_transport_started_) {
                is_sctp_transport_started_ = true;
                sctp_transport_->Start({
                    5000,
                    5000,
                    262144,
                });
            }
        }
    }

    void SctpDataChannelProviderInterfaceImpl::send_data_channel_message(const bytes::binary& data) {
        if (is_open_) {
            const std::string message = bytes::to_string(data);
            RTC_LOG(LS_VERBOSE) << "Outgoing DataChannel message: " << message;
            const webrtc::DataBuffer buffer(message);
            data_channel_->Send(buffer);
        } else {
            RTC_LOG(LS_VERBOSE) << "Could not send an outgoing DataChannel message, adding to pending messages";
            pending_messages_.push_back(data);
        }
    }

    void SctpDataChannelProviderInterfaceImpl::OnTransportClosed(const webrtc::RTCError) {
        assert(networkThread->IsCurrent());
    }

    void SctpDataChannelProviderInterfaceImpl::on_state_changed(const std::function<void(bool)>& callback) {
        on_state_changed_callback_ = callback;
    }

    void SctpDataChannelProviderInterfaceImpl::on_message_received(const std::function<void(const bytes::binary&)>& callback) {
        on_message_received_callback_ = callback;
    }
} // wrtc::interfaces
