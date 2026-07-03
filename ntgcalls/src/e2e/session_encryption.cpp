//
// Created by Lauren on 18/06/26.
//

#include <ranges>
#include <ntgcalls/e2e/session_encryption.hpp>
#include <ntgcalls/e2e/state.hpp>
#include <ntgcalls/e2e/chain/message_encryption.hpp>
#include <wrtc/utils/random.hpp>

namespace ntgcalls::e2e {
    SessionEncryption::SessionEncryption(
        const int64_t self_user_id,
        const openssl::Key25519 &private_key
    ) : self_user_id_(self_user_id), private_key_(private_key) {}

    void SessionEncryption::append_uint32(bytes::binary &buffer, const uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            buffer.push_back(static_cast<uint8_t>(value >> i * 8 & 0xff));
        }
    }

    uint32_t SessionEncryption::read_uint32(const uint8_t *data) {
        return static_cast<uint32_t>(data[0]) |
               static_cast<uint32_t>(data[1]) << 8 |
               static_cast<uint32_t>(data[2]) << 16 |
               static_cast<uint32_t>(data[3]) << 24;
    }

    void SessionEncryption::append_raw(bytes::binary &buffer, const bytes::const_span value) {
        const auto data = value.data();
        buffer.insert(buffer.end(), data, data + value.size());
    }

    bool SessionEncryption::valid_channel_id(const int32_t channel_id) {
        return channel_id >= 0 && channel_id <= 1023;
    }

    bytes::binary SessionEncryption::magic(const int32_t id) {
        bytes::binary result;
        append_uint32(result, static_cast<uint32_t>(id));
        return result;
    }

    bool SessionEncryption::check_not_seen(const tl::PublicKeyBytes& public_key, const int32_t channel_id,
        const uint32_t seqno_value) {
        const auto& s = seen_[{public_key, channel_id}];
        if (s.empty()) {
            return true;
        }
        if (seqno_value < *s.begin()) {
            return false;
        }
        return !s.contains(seqno_value);
    }

    void SessionEncryption::mark_as_seen(const tl::PublicKeyBytes& public_key, const int32_t channel_id,
        const uint32_t seqno_value) {
        auto& s = seen_[{public_key, channel_id}];
        s.insert(seqno_value);
        while (s.size() > 1024 || (!s.empty() && *s.begin() + 1024 < seqno_value)) {
            s.erase(s.begin());
        }
    }

    void SessionEncryption::sync() {
        const auto now = webrtc::TimeMillis();
        while (!epochs_to_forget_.empty() && (epochs_to_forget_.front().first <= now || epochs_.size() > kMaxActiveEpochs)) {
            const auto epoch = epochs_to_forget_.front().second;
            if (const auto it = epochs_.find(epoch); it != epochs_.end()) {
                epoch_by_hash_.erase(it->second.epoch_hash);
                epochs_.erase(it);
            }
            epochs_to_forget_.pop_front();
        }
    }

    int64_t SessionEncryption::user_id() const {
        return self_user_id_;
    }

    std::optional<bytes::binary> SessionEncryption::encrypt_packet_with_secret(
        const int32_t channel_id,
        const bytes::const_span unencrypted_part,
        const bytes::const_span packet,
        const bytes::const_span one_time_secret
    ) {
        if (!valid_channel_id(channel_id)) {
            return std::nullopt;
        }
        auto& channel_seqno = seqno_[channel_id];
        if (channel_seqno == std::numeric_limits<uint32_t>::max()) {
            return std::nullopt;
        }
        ++channel_seqno;

        bytes::binary payload;
        append_uint32(payload, static_cast<uint32_t>(channel_id));
        append_uint32(payload, channel_seqno);
        append_raw(payload, packet);

        bytes::binary extra = magic(kCallPacketMagic);
        append_raw(extra, unencrypted_part);

        std::array<uint8_t, 32> large_msg_id{};
        const auto encrypted = chain::MessageEncryption::encrypt_data(bytes::view(payload), one_time_secret, bytes::view(extra), &large_msg_id);

        bytes::binary to_sign = magic(kCallPacketLargeMsgIdMagic);
        append_raw(to_sign, bytes::view(large_msg_id));
        const auto signature = private_key_.sign(bytes::view(to_sign));

        bytes::binary result = encrypted;
        append_raw(result, bytes::view(signature));
        return result;
    }

    std::optional<bytes::binary> SessionEncryption::decrypt_packet_with_secret(
        const int64_t expected_user_id,
        const bytes::const_span unencrypted_header,
        const bytes::const_span unencrypted_prefix,
        const bytes::const_span encrypted_packet,
        const bytes::const_span one_time_secret,
        const chain::GroupState &group_state
    ) {
        const auto participant = State::find_participant(group_state, expected_user_id);
        if (!participant) {
            return std::nullopt;
        }
        if (encrypted_packet.size() < 64) {
            return std::nullopt;
        }
        const auto signature = encrypted_packet.subspan(encrypted_packet.size() - 64, 64);
        const auto payload_cipher = encrypted_packet.subspan(0, encrypted_packet.size() - 64);

        bytes::binary extra = magic(kCallPacketMagic);
        append_raw(extra, unencrypted_header);
        append_raw(extra, unencrypted_prefix);

        std::array<uint8_t, 32> large_msg_id{};
        const auto payload = chain::MessageEncryption::decrypt_data(
            payload_cipher,
            one_time_secret,
            bytes::view(extra),
            &large_msg_id
        );
        if (!payload) {
            return std::nullopt;
        }

        bytes::binary to_verify = magic(kCallPacketLargeMsgIdMagic);
        append_raw(to_verify, bytes::view(large_msg_id));
        if (!openssl::Key25519::verify(bytes::view(participant->public_key), bytes::view(to_verify), signature)) {
            return std::nullopt;
        }

        if (payload->size() < 8) {
            return std::nullopt;
        }
        const auto channel_id = static_cast<int32_t>(read_uint32(payload->data()));
        const auto seqno_value = read_uint32(payload->data() + 4);
        if (!valid_channel_id(channel_id)) {
            return std::nullopt;
        }
        if (!check_not_seen(participant->public_key, channel_id, seqno_value)) {
            return std::nullopt;
        }
        mark_as_seen(participant->public_key, channel_id, seqno_value);

        bytes::binary result(unencrypted_prefix.size() + payload->size() - 8);
        std::copy_n(unencrypted_prefix.data(), unencrypted_prefix.size(), result.begin());
        std::copy(payload->begin() + 8, payload->end(), result.begin() + static_cast<std::ptrdiff_t>(unencrypted_prefix.size()));
        return result;
    }

    std::optional<bytes::binary> SessionEncryption::encrypt(const int32_t channel_id, const bytes::const_span data, const size_t unencrypted_prefix_length) {
        sync();
        if (unencrypted_prefix_length > data.size() || unencrypted_prefix_length >= 1 << 16) {
            return std::nullopt;
        }
        if (epochs_.empty()) {
            return std::nullopt;
        }
        const auto prefix = data.subspan(0, unencrypted_prefix_length);
        const auto decrypted = data.subspan(unencrypted_prefix_length);

        bytes::binary header_a;
        append_uint32(header_a, static_cast<uint32_t>(epochs_.size()));
        for (const auto &info: epochs_ | std::views::values) {
            header_a.insert(header_a.end(), info.epoch_hash.begin(), info.epoch_hash.end());
        }

        std::array<uint8_t, 32> one_time_secret{};
        bytes::random_fill(bytes::span(one_time_secret.data(), one_time_secret.size()));

        bytes::binary unencrypted_part = header_a;
        append_raw(unencrypted_part, prefix);
        const auto encrypted_packet = encrypt_packet_with_secret(channel_id, bytes::view(unencrypted_part), decrypted, bytes::view(one_time_secret));
        if (!encrypted_packet) {
            return std::nullopt;
        }

        bytes::binary header_b;
        for (const auto &info: epochs_ | std::views::values) {
            const auto encrypted_header = chain::MessageEncryption::encrypt_header(
                bytes::view(one_time_secret),
                bytes::view(*encrypted_packet),
                bytes::view(info.secret)
            );
            if (!encrypted_header || encrypted_header->size() != 32) {
                return std::nullopt;
            }
            append_raw(header_b, bytes::view(*encrypted_header));
        }

        bytes::binary result;
        append_raw(result, prefix);
        append_raw(result, bytes::view(header_a));
        append_raw(result, bytes::view(header_b));
        append_raw(result, bytes::view(*encrypted_packet));
        append_uint32(result, static_cast<uint32_t>(unencrypted_prefix_length));
        return result;
    }

    std::optional<bytes::binary> SessionEncryption::decrypt(const int64_t user_id, const bytes::const_span packet) {
        sync();
        if (packet.size() < 4) {
            return std::nullopt;
        }
        const auto base = packet.data();
        const auto unencrypted_prefix_size = read_uint32(base + packet.size() - 4);
        const auto body = packet.subspan(0, packet.size() - 4);
        if (unencrypted_prefix_size > body.size() || unencrypted_prefix_size >= 1 << 16) {
            return std::nullopt;
        }
        const auto unencrypted_prefix = body.subspan(0, unencrypted_prefix_size);
        const auto encrypted_data = body.subspan(unencrypted_prefix_size);
        if (user_id == self_user_id_) {
            return std::nullopt;
        }
        if (encrypted_data.size() < 4) {
            return std::nullopt;
        }
        const auto enc_base = encrypted_data.data();
        const auto head = read_uint32(enc_base);
        const auto epochs_n = head & 0xff;
        const auto version = head >> 8 & 0xff;
        if (const auto reserved = head >> 16; version != 0 || reserved != 0 || epochs_n > static_cast<uint32_t>(kMaxActiveEpochs)) {
            return std::nullopt;
        }
        const size_t header_size = 4 + static_cast<size_t>(epochs_n) * 32;
        const size_t body_size = header_size + static_cast<size_t>(epochs_n) * 32;
        if (encrypted_data.size() < body_size) {
            return std::nullopt;
        }
        const auto unencrypted_header = encrypted_data.subspan(0, header_size);
        const auto encrypted_packet = encrypted_data.subspan(body_size);

        for (uint32_t i = 0; i < epochs_n; ++i) {
            tl::Hash256 epoch_hash{};
            std::copy_n(enc_base + 4 + static_cast<size_t>(i) * 32, 32, epoch_hash.begin());
            const auto header_view = encrypted_data.subspan(header_size + static_cast<size_t>(i) * 32, 32);

            const auto by_hash = epoch_by_hash_.find(epoch_hash);
            if (by_hash == epoch_by_hash_.end()) {
                continue;
            }
            const auto epoch_it = epochs_.find(by_hash->second);
            if (epoch_it == epochs_.end()) {
                continue;
            }
            const auto& info = epoch_it->second;
            const auto one_time_secret = chain::MessageEncryption::decrypt_header(header_view, encrypted_packet, bytes::view(info.secret));
            if (!one_time_secret) {
                continue;
            }
            return decrypt_packet_with_secret(
                user_id,
                unencrypted_header,
                unencrypted_prefix,
                encrypted_packet,
                bytes::view(*one_time_secret),
                info.group_state
            );
        }
        return std::nullopt;
    }

    void SessionEncryption::forget_shared_key(const int32_t epoch, const tl::Hash256 &) {
        sync();
        epochs_to_forget_.emplace_back(webrtc::TimeMillis() + kForgetEpochDelayMs, epoch);
    }

    bool SessionEncryption::add_shared_key(const int32_t epoch, const tl::Hash256 &epoch_hash, bytes::binary secret,
        chain::GroupState group_state) {
        sync();
        const auto self = State::find_participant(group_state, private_key_.public_key_bytes());
        if (!self || self->user_id != self_user_id_) {
            return false;
        }
        epoch_by_hash_[epoch_hash] = epoch;
        epochs_.insert_or_assign(epoch, EpochInfo{epoch, epoch_hash, self->user_id, std::move(secret), std::move(group_state)});
        return true;
    }
} // ntgcalls::e2e