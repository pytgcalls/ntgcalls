//
// Created by Lauren on 09/03/24.
//

#include <ntgcalls/signaling/crypto/signaling_encryption.hpp>
#include <ntgcalls/signaling/messages/message.hpp>
#include <rtc_base/copy_on_write_buffer.h>
#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>
#include <wrtc/utils/encryption.hpp>

namespace ntgcalls::signaling::crypto {
    SignalingEncryption::SignalingEncryption(EncryptionKey key):
       key_(std::move(key)),
        send_acks_timer_active_(false),
        resend_timer_active_(false) {}

    SignalingEncryption::~SignalingEncryption() {
        const std::lock_guard lock(mutex_);
        request_send_service_callback_ = nullptr;
        counter_ = 0;
        largest_incoming_counters_.clear();
    }

    bytes::binary SignalingEncryption::encrypt_prepared(const webrtc::CopyOnWriteBuffer &buffer) {
        const std::lock_guard lock(mutex_);
        bytes::binary encrypted(16 + buffer.size());
        const auto x = (key_.is_outgoing ? 0 : 8) + 128;
        const auto key = key_.value->data();
        const auto msg_key_large = openssl::Sha256::concat(
            bytes::memory_span(key + 88 + x, 32),
            bytes::memory_span(buffer.data(), buffer.size())
        );
        const auto msg_key = encrypted.data();
        std::memcpy(msg_key, msg_key_large.data() + 8, 16);
        auto aes_key_iv = openssl::Aes::prepare_key_iv(key, msg_key, x);
        openssl::Aes::process_ctr(
            bytes::memory_span(buffer.data(), buffer.size()),
            encrypted.data() + 16,
            aes_key_iv
        );
        return encrypted;
    }

    void SignalingEncryption::write_seq(void *bytes, const uint32_t seq) {
        *static_cast<uint32_t*>(bytes) = webrtc::HostToNetwork32(seq);
    }

    uint32_t SignalingEncryption::read_seq(const void* bytes) {
        return webrtc::NetworkToHost32(*static_cast<const uint32_t*>(bytes));
    }

    void SignalingEncryption::append_seq(webrtc::CopyOnWriteBuffer &buffer, const uint32_t seq) {
        const auto bytes = webrtc::HostToNetwork32(seq);
        buffer.AppendData(reinterpret_cast<const char*>(&bytes), sizeof(bytes));
    }

    uint32_t SignalingEncryption::counter_from_seq(const uint32_t seq) {
        return seq & ~kSingleMessagePacketSeqBit & ~kMessageRequiresAckSeqBit;
    }

    // ReSharper disable once CppDFAConstantParameter
    bool SignalingEncryption::const_time_is_different(const void *a, const void *b, const size_t size) {
        auto ca = static_cast<const char*>(a);
        auto cb = static_cast<const char*>(b);
        volatile auto different = false;
        for (const auto ce = ca + size; ca != ce; ++ca, ++cb) {
            different |= *ca != *cb;
        }
        return different;
    }

    bool SignalingEncryption::register_incoming_counter(const uint32_t incoming_counter) {
        auto &list = largest_incoming_counters_;
        const auto position = std::ranges::lower_bound(list, incoming_counter);
        const auto largest = list.empty() ? 0 : list.back();
        if (position != list.end() && *position == incoming_counter) {
            return false;
        }
        if (incoming_counter + kEepIncomingCountersCount <= largest) {
            return false;
        }
        const auto erase_till = std::ranges::find_if(list, [&](const uint32_t c) {
            return c + kEepIncomingCountersCount > incoming_counter;
        });
        const auto erase_count = erase_till - list.begin();
        const auto position_index = position - list.begin() - erase_count;
        list.erase(list.begin(), erase_till);

        assert(positionIndex >= 0 && positionIndex <= list.size());
        list.insert(list.begin() + position_index, incoming_counter);
        return true;
    }

    void SignalingEncryption::ack_my_message(const uint32_t seq) {
        auto type = static_cast<uint8_t>(0);
        auto &list = my_not_yet_acked_messages_;
        for (auto i = list.begin(), e = list.end(); i != e; ++i) {
            assert(i->data.size() >= 5);
            if (read_seq(i->data.cdata()) == seq) {
                type = static_cast<uint8_t>(i->data.cdata()[4]);
                list.erase(i);
                break;
            }
        }
        RTC_LOG(LS_VERBOSE) << (type ? "Got ACK:type" + std::to_string(type) + "#" : "Repeated ACK#") << counter_from_seq(seq);
    }

    void SignalingEncryption::send_ack_postponed(const uint32_t incoming_seq) {
        auto &list = acks_to_send_seqs_;
        if (const auto already = std::ranges::find(list, incoming_seq); already == list.end()) {
            list.push_back(incoming_seq);
        }
    }

    bool SignalingEncryption::register_sent_ack(const uint32_t c, const bool first_in_packet) {
        auto &list = acks_sent_counters_;
        const auto position = std::ranges::lower_bound(list, c);
        const auto already = position != list.end() && *position == c;
        const auto was = list;
        if (first_in_packet) {
            list.erase(list.begin(), position);
            if (!already) {
                list.insert(list.begin(), c);
            }
        } else if (!already) {
            list.insert(position, c);
        }
        return !already;
    }

    std::vector<webrtc::CopyOnWriteBuffer> SignalingEncryption::process_raw_packet(const webrtc::Buffer &full_buffer, const uint32_t packet_seq) {
        if (full_buffer.size() < 4) {
            RTC_LOG(LS_ERROR) << "Bad incoming data size";
            return {};
        }

        auto additional_message = false;
        auto first_message_requiring_ack = true;
        auto new_requiring_ack_received = false;

        auto current_seq = packet_seq;
        auto current_counter = counter_from_seq(current_seq);
        webrtc::ByteBufferReader reader(std::span(full_buffer.data() + 4, full_buffer.size() - 4));
        auto messages = std::vector<webrtc::CopyOnWriteBuffer>();
        while (true) {
            const auto type = static_cast<uint8_t>(*reader.Data());
            const auto single_message_packet = (current_seq & kSingleMessagePacketSeqBit) != 0;
            if (single_message_packet && additional_message) {
                RTC_LOG(LS_ERROR) << "Single message packet with additional message";
                return {};
            }
            if (type == kEmptyId) {
                if (additional_message) {
                    RTC_LOG(LS_ERROR) << "Got RECV:empty in additional message";
                    return {};
                }
                RTC_LOG(LS_VERBOSE) << "Got RECV:empty" << "#" << current_counter;
                reader.Consume(1);
            } else if (type == kAckId) {
                if (!additional_message) {
                    RTC_LOG(LS_ERROR) << "Ack message must not be the first one in the packet.";
                    return {};
                }
                ack_my_message(current_seq);
                reader.Consume(1);
            } else if (type == kCustomId) {
                reader.Consume(1);
                if (auto message = messages::Message::deserialize_raw(reader)) {
                    const auto message_requires_ack = (current_seq & kMessageRequiresAckSeqBit) != 0;
                    const auto skip_message = message_requires_ack
                        ? !register_sent_ack(current_counter, first_message_requiring_ack)
                        : additional_message && !register_incoming_counter(current_counter);
                    if (message_requires_ack) {
                        first_message_requiring_ack = false;
                        if (!skip_message) {
                            new_requiring_ack_received = true;
                        }
                        send_ack_postponed(current_seq);
                        RTC_LOG(LS_VERBOSE) << (skip_message ? "Repeated RECV:type" : "Got RECV:type") << type << "#" << current_counter;
                    }
                    if (!skip_message) {
                        messages.push_back(std::move(*message));
                    }
                } else {
                    RTC_LOG(LS_ERROR) << "Could not parse message from packet, type: " << std::to_string(type);
                    return {};
                }
            } else {
                RTC_LOG(LS_ERROR) << "Unknown message type: " << std::to_string(type);
                return {};
            }

            if (!reader.Length()) {
                break;
            }
            if (single_message_packet) {
                RTC_LOG(LS_ERROR) << "Single message didn't fill the entire packet.";
                return {};
            }
            if (reader.Length() < 5) {
                RTC_LOG(LS_ERROR) << "Bad remaining data size: " << std::to_string(reader.Length());
                return {};
            }
            const auto success = reader.ReadUInt32(&current_seq);
            assert(success);
            (void) success;
            current_counter = counter_from_seq(current_seq);
            additional_message = true;
        }
        if (!acks_to_send_seqs_.empty()) {
            if (new_requiring_ack_received) {
                (void) request_send_service_callback_(0, 0);
            } else if (!send_acks_timer_active_) {
                send_acks_timer_active_ = true;
                (void) request_send_service_callback_(kMaxDelayBeforeAckResend, kServiceCauseAcks);
            }
        }
        return messages;
    }

    std::optional<uint32_t> SignalingEncryption::compute_next_seq(const bool message_requires_ack) {
        if (message_requires_ack && my_not_yet_acked_messages_.size() >= kNotAckedMessagesLimit) {
            RTC_LOG(LS_ERROR) << "Too many not ACKed messages.";
            return std::nullopt;
        }
        if (counter_ == kMaxAllowedCounter) {
            RTC_LOG(LS_ERROR) << "Outgoing packet limit reached.";
            return std::nullopt;
        }

        return static_cast<uint32_t>(++counter_ | (message_requires_ack ? kMessageRequiresAckSeqBit : 0));
    }

    bool SignalingEncryption::enough_space_in_packet(const webrtc::CopyOnWriteBuffer &buffer, const size_t amount) {
        return amount < kMaxSignalingPacketSize && 16 + buffer.size() + amount <= kMaxSignalingPacketSize;
    }

    webrtc::CopyOnWriteBuffer SignalingEncryption::serialize_empty_message_with_seq(const uint32_t seq) {
        auto result = webrtc::CopyOnWriteBuffer(5);
        const auto bytes = result.MutableData();
        write_seq(bytes, seq);
        bytes[4] = kEmptyId;
        return result;
    }

    webrtc::CopyOnWriteBuffer SignalingEncryption::serialize_raw_message_with_seq(const webrtc::CopyOnWriteBuffer &message, const uint32_t seq) {
        webrtc::ByteBufferWriter writer;
        writer.WriteUInt32(seq);
        writer.WriteUInt8(kCustomId);
        writer.WriteUInt32(static_cast<uint32_t>(message.size()));
        writer.Write(std::span(message.data(), message.size()));
        auto result = webrtc::CopyOnWriteBuffer();
        result.AppendData(writer.Data(), writer.Length());
        return result;
    }

    void SignalingEncryption::append_messages(webrtc::CopyOnWriteBuffer &buffer) {
        append_acks_to_send(buffer);

        if (my_not_yet_acked_messages_.empty()) {
            return;
        }
        const auto now = webrtc::TimeMillis();
        for (auto &[data, lastSent] : my_not_yet_acked_messages_) {
            const auto sent = lastSent;
            const auto when = sent ? sent + kMinDelayBeforeMessageResend : 0;
            assert(data.size() >= 5);
            const auto c = counter_from_seq(read_seq(data.data()));
            const auto type = static_cast<uint8_t>(data.data()[4]);
            if (when > now) {
                RTC_LOG(LS_VERBOSE)<< "Skip RESEND:type" << type << "#" << c << " (wait " << when - now << "ms).";
                break;
            }
            if (enough_space_in_packet(buffer, data.size())) {
                RTC_LOG(LS_VERBOSE) << "Add RESEND:type" << type << "#" << c;
                buffer.AppendData(data);
                lastSent = now;
            } else {
                RTC_LOG(LS_VERBOSE) << "Skip RESEND:type" << type << "#" << c << " (no space, length: " << data.size() << ", already: " << buffer.size() << ")";
                break;
            }
        }

        if (!resend_timer_active_) {
            resend_timer_active_ = true;
            (void) request_send_service_callback_(
                kMaxDelayBeforeMessageResend,
                kServiceCauseResend
            );
        }
    }

    void SignalingEncryption::append_acks_to_send(webrtc::CopyOnWriteBuffer &buffer) {
        auto i = acks_to_send_seqs_.begin();
        while (i != acks_to_send_seqs_.end() && enough_space_in_packet(buffer, kAckSerializedSize)) {
            RTC_LOG(LS_VERBOSE) << "Add ACK#" << counter_from_seq(*i);
            append_seq(buffer, *i);
            buffer.AppendData(&kAckId, 1);
            ++i;
        }
        acks_to_send_seqs_.erase(acks_to_send_seqs_.begin(), i);
        for (const auto seq : acks_to_send_seqs_) {
            RTC_LOG(LS_VERBOSE) << "Skip ACK#" << counter_from_seq(seq) << " (no space, length: " << kAckSerializedSize << ", already: " << buffer.size() << ")";
        }
    }

    bool SignalingEncryption::have_messages() const {
        return !my_not_yet_acked_messages_.empty() || !acks_to_send_seqs_.empty();
    }

    std::optional<bytes::binary> SignalingEncryption::prepare_for_sending_message_internal(webrtc::CopyOnWriteBuffer &serialized, uint32_t seq) {
        if (!enough_space_in_packet(serialized, 0)) {
            RTC_LOG(LS_ERROR) << "Too large packet: " << std::to_string(serialized.size());
            return std::nullopt;
        }
        const auto not_yet_acked_copy = serialized;
        const auto type = static_cast<uint8_t>(serialized.cdata()[4]);
        const auto send_enqueued = !my_not_yet_acked_messages_.empty();
        if (send_enqueued) {
            RTC_LOG(LS_VERBOSE) << "Enqueue SEND:type" << type << "#" << counter_from_seq(seq);
        } else {
            RTC_LOG(LS_VERBOSE) << "Add SEND:type" << type << "#" << counter_from_seq(seq);
            append_messages(serialized);
        }
        my_not_yet_acked_messages_.push_back({not_yet_acked_copy, webrtc::TimeMillis()});
        if (!send_enqueued) {
            return encrypt_prepared(serialized);
        }
        for (auto &[data, lastSent] : my_not_yet_acked_messages_) {
            lastSent = 0;
        }
        return prepare_for_sending_service(0);
    }

    std::optional<bytes::binary> SignalingEncryption::encrypt(const webrtc::CopyOnWriteBuffer &buffer, const bool is_raw) {
        if (is_raw) {
            const auto maybe_seq = compute_next_seq(true);
            if (!maybe_seq) {
                return std::nullopt;
            }
            const auto seq = *maybe_seq;
            auto serialized = serialize_raw_message_with_seq(buffer, seq);
            return prepare_for_sending_message_internal(serialized, seq);
        }
        const auto seq = ++counter_;
        webrtc::ByteBufferWriter writer;
        writer.WriteUInt32(seq);
        auto result = webrtc::CopyOnWriteBuffer();
        result.AppendData(writer.Data(), writer.Length());
        result.AppendData(buffer);
        return encrypt_prepared(result);
    }

    std::vector<webrtc::CopyOnWriteBuffer> SignalingEncryption::decrypt(const webrtc::CopyOnWriteBuffer &buffer, const bool is_raw) {
        if (buffer.size() < 21 || buffer.size() > kMaxIncomingPacketSize) {
            RTC_LOG(LS_ERROR) << "Bad incoming data size";
            return {};
        }
        const auto x = (key_.is_outgoing ? 8 : 0) + 128;
        const auto key = key_.value->data();
        const auto msg_key = buffer.data();
        const auto encrypted_data = msg_key + 16;
        const auto data_size = buffer.size() - 16;

        auto aes_key_iv = openssl::Aes::prepare_key_iv(key, msg_key, x);
        auto decryption_buffer = webrtc::Buffer::CreateUninitializedWithSize(data_size);
        openssl::Aes::process_ctr(
            bytes::memory_span(encrypted_data, data_size),
            decryption_buffer.data(),
            aes_key_iv
        );

        if (const auto msg_key_large = openssl::Sha256::concat(
            bytes::memory_span(key + 88 + x, 32),
            bytes::memory_span(decryption_buffer.data(), decryption_buffer.size())
        ); const_time_is_different(msg_key_large.data() + 8, msg_key, 16)) {
            RTC_LOG(LS_ERROR) << "Bad incoming data hash";
            return {};
        }

        const auto incoming_seq = read_seq(decryption_buffer.data());
        if (const auto incoming_counter = counter_from_seq(incoming_seq); !register_incoming_counter(incoming_counter)) {
            RTC_LOG(LS_ERROR) << "Already handled packet received." << std::to_string(incoming_counter);
            return {};
        }

        if (is_raw) {
            return process_raw_packet(decryption_buffer, incoming_seq);
        }
        webrtc::CopyOnWriteBuffer result_buffer;
        result_buffer.AppendData(decryption_buffer.data() + 4, decryption_buffer.size() - 4);
        return {result_buffer};
    }

    void SignalingEncryption::on_service_message(const std::function<void(int delay_ms, int cause)> &request_send_service) {
        request_send_service_callback_ = request_send_service;
    }

    std::optional<bytes::binary> SignalingEncryption::prepare_for_sending_service(const int cause) {
        if (cause == kServiceCauseAcks) {
            send_acks_timer_active_ = false;
        } else if (cause == kServiceCauseResend) {
            resend_timer_active_ = false;
        }
        if (!have_messages()) {
            return std::nullopt;
        }
        const auto seq = compute_next_seq(false);
        if (!seq) {
            return std::nullopt;
        }
        auto serialized = serialize_empty_message_with_seq(*seq);
        if (!enough_space_in_packet(serialized, 0)) {
            RTC_LOG(LS_ERROR) << "Failed to serialize empty message";
            return std::nullopt;
        }
        RTC_LOG(LS_VERBOSE) << "SEND:empty#" << counter_from_seq(*seq);
        append_messages(serialized);
        return encrypt_prepared(serialized);
    }
} // ntgcalls::signaling::crypto