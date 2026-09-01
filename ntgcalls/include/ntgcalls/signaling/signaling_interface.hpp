//
// Created by Lauren on 16/03/24.
//

#pragma once
#include <vector>
#include <ntgcalls/signaling/crypto/signaling_encryption.hpp>
#include <wrtc/utils/safe_thread.hpp>

namespace ntgcalls::signaling {
    using DataEmitter = std::function<void(const bytes::binary&)>;
    using DataReceiver = std::function<void(const std::vector<bytes::binary>&)>;

    class SignalingInterface: public std::enable_shared_from_this<SignalingInterface> {
        std::mutex mutex_;
        std::unique_ptr<crypto::SignalingEncryption> signaling_encryption_;

    protected:
        DataReceiver on_signal_data_;
        DataEmitter on_emit_data_;
        wrtc::utils::SafeThread &network_thread_, &signaling_thread_;

        std::vector<bytes::binary> pre_read_data(const bytes::binary& data, bool is_raw = false);

        bytes::binary pre_send_data(const bytes::binary& data, bool is_raw = false);

        virtual bool supports_compression() const = 0;

    public:
        virtual ~SignalingInterface() = default;

        SignalingInterface(
            wrtc::utils::SafeThread& network_thread,
            wrtc::utils::SafeThread& signaling_thread,
            const crypto::EncryptionKey& key,
            DataEmitter on_emit_data,
            DataReceiver on_signal_data
        );

        void init();

        virtual void send(const bytes::binary& data) = 0;

        virtual void receive(const bytes::binary& data) = 0;

        virtual void close();
    };
} // ntgcalls::signaling
