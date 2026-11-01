//
// Created by Lauren on 16/03/24.
//

#include <api/environment/environment_factory.h>
#include <media/sctp/dcsctp_transport.h>
#include <net/dcsctp/socket/dcsctp_socket.h>
#include <ntgcalls/signaling/signaling_sctp_connection.hpp>

namespace ntgcalls::signaling {
    std::unique_ptr<dcsctp::DcSctpSocketInterface> SignalingSctpConnection::SocketFactory::Create(absl::string_view log_prefix, dcsctp::DcSctpSocketCallbacks& callbacks, std::unique_ptr<dcsctp::PacketObserver> packet_observer, const dcsctp::DcSctpOptions& options) {
        dcsctp::DcSctpOptions custom_options = options;
        custom_options.t1_init_timeout = dcsctp::DurationMs(400);
        custom_options.t1_cookie_timeout = dcsctp::DurationMs(400);
        custom_options.max_timer_backoff_duration = dcsctp::DurationMs(750);
        return std::make_unique<dcsctp::DcSctpSocket>(log_prefix, callbacks, std::move(packet_observer), custom_options);
    }

    SignalingSctpConnection::SignalingSctpConnection(
        wrtc::utils::SafeThread& network_thread,
        wrtc::utils::SafeThread& signaling_thread,
        const webrtc::Environment& env,
        const crypto::EncryptionKey& key,
        const DataEmitter& on_emit_data,
        const DataReceiver& on_signal_data,
        const bool allow_compression
    ): SignalingInterface(network_thread, signaling_thread, key, on_emit_data, on_signal_data), allow_compression_(allow_compression) {
        network_thread.BlockingCall([&] {
            packet_transport_ = std::make_unique<SignalingPacketTransport>(on_emit_data, key.is_outgoing);
            sctp_transport_ = std::make_unique<webrtc::DcSctpTransport>(env, network_thread, packet_transport_.get(), std::make_unique<SocketFactory>());
            sctp_transport_->OpenStream(0, webrtc::PriorityValue(webrtc::Priority::kVeryLow));
            sctp_transport_->SetDataChannelSink(this);
            sctp_transport_->Start({
                5000,
                5000,
                262144,
            });
        });
    }
    void SignalingSctpConnection::close() {
        SignalingInterface::close();
        network_thread_.BlockingCall([&] {
            sctp_transport_ = nullptr;
            packet_transport_ = nullptr;
        });
    }

    void SignalingSctpConnection::receive(const bytes::binary& data) {
        network_thread_.BlockingCall([&] {
            packet_transport_->set_writable(true);
            packet_transport_->receive_data(data);
        });
    }

    void SignalingSctpConnection::send(const bytes::binary& data) {
        network_thread_.BlockingCall([&] {
            const auto encrypted_data = pre_send_data(data);
            if (is_ready_to_send_) {
                webrtc::SendDataParams params;
                params.type = webrtc::DataMessageType::kBinary;
                params.ordered = true;

                webrtc::CopyOnWriteBuffer payload;
                payload.AppendData(encrypted_data.data(), encrypted_data.size());

                if (const auto result = sctp_transport_->SendData(0, params, payload); !result.ok()) {
                    RTC_LOG(LS_ERROR) << "Failed to send data: " << result.message();
                    is_ready_to_send_ = false;
                    pending_data_.push_back(encrypted_data);
                }
            } else {
                pending_data_.push_back(encrypted_data);
            }
        });
    }

    void SignalingSctpConnection::OnReadyToSend() {
        assert(networkThread.IsCurrent());
        is_ready_to_send_ = true;
        auto pending_data = std::move(pending_data_);
        pending_data_.clear();
        for (auto it = pending_data.begin(); it != pending_data.end(); ++it) {
            webrtc::SendDataParams params;
            params.type = webrtc::DataMessageType::kBinary;
            params.ordered = true;
            webrtc::CopyOnWriteBuffer payload;
            payload.AppendData(it->data(), it->size());
            if (const auto result = sctp_transport_->SendData(0, params, payload); !result.ok()) {
                RTC_LOG(LS_ERROR) << "Failed to send data: " << result.message();
                pending_data_.assign(std::make_move_iterator(it), std::make_move_iterator(pending_data.end()));
                is_ready_to_send_ = false;
                break;
            }
        }
    }

    void SignalingSctpConnection::OnDataReceived(int channel_id, webrtc::DataMessageType type, const webrtc::CopyOnWriteBuffer& buffer) {
        assert(networkThread.IsCurrent());
        const auto signal_data_callback = on_signal_data_;
        const auto decrypted_data = pre_read_data({buffer.data(), buffer.data() + buffer.size()});
        signaling_thread_.PostTask([signal_data_callback, decrypted_data] {
            if (signal_data_callback) {
                signal_data_callback(decrypted_data);
            }
        });
    }

    void SignalingSctpConnection::OnTransportClosed(webrtc::RTCError error) {
        assert(networkThread.IsCurrent());
    }

    bool SignalingSctpConnection::supports_compression() const {
        return allow_compression_;
    }
} // ntgcalls::signaling
