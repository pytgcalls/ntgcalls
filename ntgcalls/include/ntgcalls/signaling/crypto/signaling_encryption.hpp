//
// Created by Laky64 on 09/03/2024.
//
#pragma once
#include <cstdint>
#include <optional>
#include <rtc_base/byte_buffer.h>
#include <rtc_base/copy_on_write_buffer.h>


#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/syncronized_callback.hpp>
#include <ntgcalls/signaling/crypto/auth_key.hpp>

namespace signaling {

    class SignalingEncryption {
        struct MessageForResend {
            rtc::CopyOnWriteBuffer data;
            int64_t lastSent = 0;
        };

        uint64_t counter = 0;
        std::mutex mutex;
        EncryptionKey _key;
        std::vector<uint32_t> largestIncomingCounters;
        std::vector<MessageForResend> myNotYetAckedMessages;
        std::vector<uint32_t> acksSentCounters;
        std::vector<uint32_t> acksToSendSeqs;
        bool sendAcksTimerActive, resendTimerActive;

        static constexpr auto kSingleMessagePacketSeqBit = static_cast<uint32_t>(1) << 31;
        static constexpr auto kMessageRequiresAckSeqBit = static_cast<uint32_t>(1) << 30;
        static constexpr auto kMaxAllowedCounter = std::numeric_limits<uint32_t>::max() & ~kSingleMessagePacketSeqBit & ~kMessageRequiresAckSeqBit;
        static constexpr auto kKeepIncomingCountersCount = 64;
        static constexpr auto kMaxSignalingPacketSize = 16 * 1024;
        static constexpr auto kMaxIncomingPacketSize = 128 * 1024;
        static constexpr auto kAckSerializedSize = sizeof(uint32_t) + sizeof(uint8_t);
        static constexpr auto kNotAckedMessagesLimit = 64 * 1024;

        static constexpr auto minDelayBeforeMessageResend = 3000;
        static constexpr auto maxDelayBeforeAckResend = 5000;
        static constexpr auto maxDelayBeforeMessageResend = 5000;

        static constexpr auto kAckId = static_cast<uint8_t>(-1);
        static constexpr auto kEmptyId = static_cast<uint8_t>(-2);
        static constexpr auto kCustomId = static_cast<uint8_t>(127);
        static constexpr auto kServiceCauseAcks = 1;
        static constexpr auto kServiceCauseResend = 2;

        wrtc::synchronized_callback<int, int> requestSendServiceCallback;

        [[nodiscard]] bytes::binary encryptPrepared(const rtc::CopyOnWriteBuffer &buffer);

        static void WriteSeq(void *bytes, uint32_t seq);

        static uint32_t ReadSeq(const void* bytes);

        static void AppendSeq(rtc::CopyOnWriteBuffer &buffer, uint32_t seq);

        static uint32_t CounterFromSeq(uint32_t seq);

        static bool ConstTimeIsDifferent(const void *a, const void *b, size_t size);

        bool registerIncomingCounter(uint32_t incomingCounter);

        void ackMyMessage(uint32_t seq);

        void sendAckPostponed(uint32_t incomingSeq);

        bool registerSentAck(uint32_t counter, bool firstInPacket);

        std::vector<rtc::CopyOnWriteBuffer> processRawPacket(const rtc::Buffer &fullBuffer,uint32_t packetSeq);

        std::optional<uint32_t> computeNextSeq(bool messageRequiresAck);

        static bool enoughSpaceInPacket(const rtc::CopyOnWriteBuffer &buffer, size_t amount);

        static rtc::CopyOnWriteBuffer SerializeEmptyMessageWithSeq(uint32_t seq);

        static rtc::CopyOnWriteBuffer SerializeRawMessageWithSeq(const rtc::CopyOnWriteBuffer &message, uint32_t seq);

        void appendMessages(rtc::CopyOnWriteBuffer &buffer);

        void appendAcksToSend(rtc::CopyOnWriteBuffer &buffer);

        bool haveMessages() const;

        std::optional<bytes::binary> prepareForSendingMessageInternal(rtc::CopyOnWriteBuffer &serialized, uint32_t seq);

    public:
        explicit SignalingEncryption(EncryptionKey key);

        ~SignalingEncryption();

        std::optional<bytes::binary> encrypt(const rtc::CopyOnWriteBuffer &buffer, bool isRaw);

        std::vector<rtc::CopyOnWriteBuffer> decrypt(const rtc::CopyOnWriteBuffer &buffer, bool isRaw);

        void onServiceMessage(const std::function<void(int delayMs, int cause)> &requestSendService);

        std::optional<bytes::binary> prepareForSendingService(int cause);
    };

} // signaling

