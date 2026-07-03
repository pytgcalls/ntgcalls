//
// Created by Lauren on 09/03/24.
//
#pragma once
#include <optional>
#include <ntgcalls/signaling/crypto/auth_key.hpp>
#include <rtc_base/copy_on_write_buffer.h>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace ntgcalls::signaling::crypto {

    class SignalingEncryption {
        struct MessageForResend {
            webrtc::CopyOnWriteBuffer data;
            int64_t last_sent = 0;
        };

        uint64_t counter_ = 0;
        std::mutex mutex_;
        EncryptionKey key_;
        std::vector<uint32_t> largest_incoming_counters_;
        std::vector<MessageForResend> my_not_yet_acked_messages_;
        std::vector<uint32_t> acks_sent_counters_;
        std::vector<uint32_t> acks_to_send_seqs_;
        bool send_acks_timer_active_, resend_timer_active_;

        static constexpr auto kSingleMessagePacketSeqBit = static_cast<uint32_t>(1) << 31;
        static constexpr auto kMessageRequiresAckSeqBit = static_cast<uint32_t>(1) << 30;
        static constexpr auto kMaxAllowedCounter = std::numeric_limits<uint32_t>::max() & ~kSingleMessagePacketSeqBit & ~kMessageRequiresAckSeqBit;
        static constexpr auto kEepIncomingCountersCount = 64;
        static constexpr auto kMaxSignalingPacketSize = 16 * 1024;
        static constexpr auto kMaxIncomingPacketSize = 128 * 1024;
        static constexpr auto kAckSerializedSize = sizeof(uint32_t) + sizeof(uint8_t);
        static constexpr auto kNotAckedMessagesLimit = 64 * 1024;

        static constexpr auto kMinDelayBeforeMessageResend = 3000;
        static constexpr auto kMaxDelayBeforeAckResend = 5000;
        static constexpr auto kMaxDelayBeforeMessageResend = 5000;

        static constexpr auto kAckId = static_cast<uint8_t>(-1);
        static constexpr auto kEmptyId = static_cast<uint8_t>(-2);
        static constexpr auto kCustomId = static_cast<uint8_t>(127);
        static constexpr auto kServiceCauseAcks = 1;
        static constexpr auto kServiceCauseResend = 2;

        wrtc::utils::synchronized_callback<void(int, int)> request_send_service_callback_;

        [[nodiscard]] bytes::binary encrypt_prepared(const webrtc::CopyOnWriteBuffer &buffer);

        static void write_seq(void *bytes, uint32_t seq);

        static uint32_t read_seq(const void* bytes);

        static void append_seq(webrtc::CopyOnWriteBuffer &buffer, uint32_t seq);

        static uint32_t counter_from_seq(uint32_t seq);

        static bool const_time_is_different(const void *a, const void *b, size_t size);

        bool register_incoming_counter(uint32_t incoming_counter);

        void ack_my_message(uint32_t seq);

        void send_ack_postponed(uint32_t incoming_seq);

        bool register_sent_ack(uint32_t counter, bool first_in_packet);

        std::vector<webrtc::CopyOnWriteBuffer> process_raw_packet(const webrtc::Buffer &full_buffer, uint32_t packet_seq);

        std::optional<uint32_t> compute_next_seq(bool message_requires_ack);

        static bool enough_space_in_packet(const webrtc::CopyOnWriteBuffer &buffer, size_t amount);

        static webrtc::CopyOnWriteBuffer serialize_empty_message_with_seq(uint32_t seq);

        static webrtc::CopyOnWriteBuffer serialize_raw_message_with_seq(const webrtc::CopyOnWriteBuffer &message, uint32_t seq);

        void append_messages(webrtc::CopyOnWriteBuffer &buffer);

        void append_acks_to_send(webrtc::CopyOnWriteBuffer &buffer);

        bool have_messages() const;

        std::optional<bytes::binary> prepare_for_sending_message_internal(webrtc::CopyOnWriteBuffer &serialized, uint32_t seq);

    public:
        explicit SignalingEncryption(EncryptionKey key);

        ~SignalingEncryption();

        std::optional<bytes::binary> encrypt(const webrtc::CopyOnWriteBuffer &buffer, bool is_raw);

        std::vector<webrtc::CopyOnWriteBuffer> decrypt(const webrtc::CopyOnWriteBuffer &buffer, bool is_raw);

        void on_service_message(const std::function<void(int delay_ms, int cause)> &request_send_service);

        std::optional<bytes::binary> prepare_for_sending_service(int cause);
    };

} // ntgcalls::signaling::crypto

