//
// Created by Lauren on 16/03/24.
//

#include <utility>
#include <ntgcalls/signaling/signaling_interface.hpp>
#include <wrtc/utils/g_zip.hpp>

namespace ntgcalls::signaling {
    void SignalingInterface::close() {
        signaling_encryption_->on_service_message(nullptr);
        on_emit_data_ = nullptr;
        on_signal_data_ = nullptr;
        signaling_thread_.BlockingCall([&] {});
        signaling_encryption_ = nullptr;
    }

    SignalingInterface::SignalingInterface(
        wrtc::utils::SafeThread& network_thread,
        wrtc::utils::SafeThread& signaling_thread,
        const crypto::EncryptionKey& key,
        DataEmitter on_emit_data,
        DataReceiver on_signal_data
    ): on_signal_data_(std::move(on_signal_data)), on_emit_data_(std::move(on_emit_data)), network_thread_(network_thread), signaling_thread_(signaling_thread) {
        signaling_encryption_ = std::make_unique<crypto::SignalingEncryption>(key);
    }

    void SignalingInterface::init() {
        const std::weak_ptr weak(shared_from_this());
        signaling_encryption_->on_service_message([weak](const int delay_ms, int cause) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            if (delay_ms == 0) {
                strong->signaling_thread_.PostTask([weak, cause] {
                    const auto strong_thread = weak.lock();
                    if (!strong_thread) {
                        return;
                    }
                    const std::lock_guard lock(strong_thread->mutex_);
                    if (const auto service = strong_thread->signaling_encryption_->prepare_for_sending_service(cause)) {
                        strong_thread->on_emit_data_(*service);
                    }
                });
            } else {
                strong->signaling_thread_.PostDelayedTask(
                    [weak, cause] {
                        const auto strong_thread = weak.lock();
                        if (!strong_thread) {
                            return;
                        }
                        const std::lock_guard lock(strong_thread->mutex_);
                        if (const auto service = strong_thread->signaling_encryption_->prepare_for_sending_service(cause)) {
                            strong_thread->on_emit_data_(*service);
                        }
                    },
                    webrtc::TimeDelta::Millis(delay_ms)
                );
            }
        });
    }

    std::vector<bytes::binary> SignalingInterface::pre_read_data(const bytes::binary& data, const bool is_raw) {
        const std::lock_guard lock(mutex_);
        RTC_LOG(LS_VERBOSE) << "Decrypting packets";
        const auto raw = signaling_encryption_->decrypt(webrtc::CopyOnWriteBuffer(data.data(), data.size()), is_raw);
        if (raw.empty()) {
            return {};
        }
        RTC_LOG(LS_VERBOSE) << "Packets decrypted";
        std::vector<bytes::binary> packets;
        for (auto& packet : raw) {
            auto decrypted_data = bytes::binary(packet.data(), packet.data() + packet.size());
            if (bytes::GZip::is_gzip(decrypted_data)) {
                RTC_LOG(LS_VERBOSE) << "Decompressing packet";
                if (auto unzipped = bytes::GZip::unzip(decrypted_data, 2 * 1024 * 1024); unzipped.has_value()) {
                    packets.push_back(unzipped.value());
                    continue;
                }
                RTC_LOG(LS_ERROR) << "Failed to decompress packet";
                continue;
            }
            packets.push_back(decrypted_data);
        }
        return packets;
    }

    bytes::binary SignalingInterface::pre_send_data(const bytes::binary& data, const bool is_raw) {
        const std::lock_guard lock(mutex_);
        auto packet_data = data;
        if (supports_compression()) {
            RTC_LOG(LS_VERBOSE) << "Compressing packet";
            packet_data = std::move(bytes::GZip::zip(packet_data));
        }
        RTC_LOG(LS_VERBOSE) << "Encrypting packet";
        const auto packet = signaling_encryption_->encrypt(webrtc::CopyOnWriteBuffer(packet_data.data(), packet_data.size()), is_raw);
        if (!packet.has_value()) {
            RTC_LOG(LS_ERROR) << "Failed to encrypt packet";
            return {};
        }
        RTC_LOG(LS_VERBOSE) << "Packet encrypted";
        return *packet;
    }
} // ntgcalls::signaling
