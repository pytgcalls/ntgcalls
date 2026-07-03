//
// Created by Lauren on 18/08/24.
//

#include <ntgcalls/signaling/external_signaling_connection.hpp>

namespace ntgcalls::signaling {
    ExternalSignalingConnection::ExternalSignalingConnection(
        wrtc::utils::SafeThread& network_thread,
        wrtc::utils::SafeThread& signaling_thread,
        const crypto::EncryptionKey &key,
        const DataEmitter& on_emit_data,
        const DataReceiver& on_signal_data
    ): SignalingInterface(network_thread, signaling_thread, key, on_emit_data, on_signal_data) {}

    void ExternalSignalingConnection::send(const bytes::binary& data) {
        on_emit_data_(pre_send_data(data, true));
    }

    void ExternalSignalingConnection::receive(const bytes::binary& data) {
        const auto signal_data_callback = on_signal_data_;
        const auto decrypted_data = pre_read_data(data, true);
        signaling_thread_.PostTask([signal_data_callback, decrypted_data] {
            if (signal_data_callback) {
                signal_data_callback(decrypted_data);
            }
        });
    }

    bool ExternalSignalingConnection::supports_compression() const {
        return false;
    }
} // ntgcalls::signaling