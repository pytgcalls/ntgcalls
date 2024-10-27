//
// Created by Laky64 on 09/03/2024.
//

#include <ntgcalls/signaling/crypto/signaling_encryption.hpp>

#include <rtc_base/copy_on_write_buffer.h>

#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>

#include <ntgcalls/signaling/messages/message.hpp>
#include <wrtc/utils/encryption.hpp>

namespace signaling {
    SignalingEncryption::SignalingEncryption(EncryptionKey key): _key(std::move(key)) {}

    SignalingEncryption::~SignalingEncryption() {
        std::lock_guard lock(mutex);
        counter = 0;
        largestIncomingCounters.clear();
    }

    bytes::binary SignalingEncryption::encryptPrepared(const rtc::CopyOnWriteBuffer &buffer) {
        std::lock_guard lock(mutex);
        bytes::binary encrypted(16 + buffer.size());
        const auto x = (_key.isOutgoing ? 0 : 8) + 128;
        const auto key = _key.value->data();
        const auto msgKeyLarge = openssl::Sha256::Concat(
            bytes::memory_span(key + 88 + x, 32),
            bytes::memory_span(buffer.data(), buffer.size())
        );
        const auto msgKey = encrypted.data();
        memcpy(msgKey, msgKeyLarge.data() + 8, 16);
        auto aesKeyIv = openssl::Aes::PrepareKeyIv(key, msgKey, x);
        openssl::Aes::ProcessCtr(
            bytes::memory_span(buffer.data(), buffer.size()),
            encrypted.data() + 16,
            aesKeyIv
        );
        return encrypted;
    }

    void SignalingEncryption::WriteSeq(void *bytes, const uint32_t seq) {
        *static_cast<uint32_t*>(bytes) = rtc::HostToNetwork32(seq);
    }

    uint32_t SignalingEncryption::ReadSeq(const void* bytes) {
        return rtc::NetworkToHost32(*static_cast<const uint32_t*>(bytes));
    }

    void SignalingEncryption::AppendSeq(rtc::CopyOnWriteBuffer &buffer, uint32_t seq) {
        const auto bytes = rtc::HostToNetwork32(seq);
        buffer.AppendData(reinterpret_cast<const char*>(&bytes), sizeof(bytes));
    }

    uint32_t SignalingEncryption::CounterFromSeq(const uint32_t seq) {
        return seq & ~kSingleMessagePacketSeqBit & ~kMessageRequiresAckSeqBit;
    }

    // ReSharper disable once CppDFAConstantParameter
    bool SignalingEncryption::ConstTimeIsDifferent(const void *a, const void *b, const size_t size) {
        auto ca = static_cast<const char*>(a);
        auto cb = static_cast<const char*>(b);
        volatile auto different = false;
        for (const auto ce = ca + size; ca != ce; ++ca, ++cb) {
            different |= *ca != *cb;
        }
        return different;
    }

    bool SignalingEncryption::registerIncomingCounter(const uint32_t incomingCounter) {
        auto &list = largestIncomingCounters;
        const auto position = std::ranges::lower_bound(list, incomingCounter);
        const auto largest = list.empty() ? 0 : list.back();
        if (position != list.end() && *position == incomingCounter) {
            return false;
        }
        if (incomingCounter + kKeepIncomingCountersCount <= largest) {
            return false;
        }
        const auto eraseTill = std::ranges::find_if(list, [&](const uint32_t counter) {
            return counter + kKeepIncomingCountersCount > incomingCounter;
        });
        const auto eraseCount = eraseTill - list.begin();
        const auto positionIndex = position - list.begin() - eraseCount;
        list.erase(list.begin(), eraseTill);

        assert(positionIndex >= 0 && positionIndex <= list.size());
        list.insert(list.begin() + positionIndex, incomingCounter);
        return true;
    }

    void SignalingEncryption::ackMyMessage(const uint32_t seq) {
        auto type = static_cast<uint8_t>(0);
        auto &list = myNotYetAckedMessages;
        for (auto i = list.begin(), e = list.end(); i != e; ++i) {
            assert(i->data.size() >= 5);
            if (ReadSeq(i->data.cdata()) == seq) {
                type = static_cast<uint8_t>(i->data.cdata()[4]);
                list.erase(i);
                break;
            }
        }
        RTC_LOG(LS_INFO) << (type ? "Got ACK:type" + std::to_string(type) + "#" : "Repeated ACK#") << CounterFromSeq(seq);
    }

    void SignalingEncryption::sendAckPostponed(const uint32_t incomingSeq) {
        auto &list = acksToSendSeqs;
        if (const auto already = std::ranges::find(list, incomingSeq); already == list.end()) {
            list.push_back(incomingSeq);
        }
    }

    bool SignalingEncryption::registerSentAck(const uint32_t counter, const bool firstInPacket) {
        auto &list = acksSentCounters;
        const auto position = std::ranges::lower_bound(list, counter);
        const auto already = position != list.end() && *position == counter;
        const auto was = list;
        if (firstInPacket) {
            list.erase(list.begin(), position);
            if (!already) {
                list.insert(list.begin(), counter);
            }
        } else if (!already) {
            list.insert(position, counter);
        }
        return !already;
    }

    std::vector<rtc::CopyOnWriteBuffer> SignalingEncryption::processRawPacket(const rtc::Buffer &fullBuffer, uint32_t packetSeq) {
        if (fullBuffer.size() < 4) {
            RTC_LOG(LS_ERROR) << "Bad incoming data size";
            return {};
        }

        auto additionalMessage = false;
        auto firstMessageRequiringAck = true;
        auto newRequiringAckReceived = false;

        auto currentSeq = packetSeq;
        auto currentCounter = CounterFromSeq(currentSeq);
        rtc::ByteBufferReader reader(rtc::MakeArrayView(fullBuffer.data() + 4, fullBuffer.size() - 4));
        auto messages = std::vector<rtc::CopyOnWriteBuffer>();
        while (true) {
            const auto type = static_cast<uint8_t>(*reader.Data());
            const auto singleMessagePacket = (currentSeq & kSingleMessagePacketSeqBit) != 0;
            if (singleMessagePacket && additionalMessage) {
                RTC_LOG(LS_ERROR) << "Single message packet with additional message";
                return {};
            }
            if (type == kEmptyId) {
                if (additionalMessage) {
                    RTC_LOG(LS_ERROR) << "Got RECV:empty in additional message";
                    return {};
                }
                RTC_LOG(LS_INFO) << "Got RECV:empty" << "#" << currentCounter;
                reader.Consume(1);
            } else if (type == kAckId) {
                if (!additionalMessage) {
                    RTC_LOG(LS_ERROR) << "Ack message must not be the first one in the packet.";
                    return {};
                }
                ackMyMessage(currentSeq);
                reader.Consume(1);
            } else if (type == kCustomId) {
                reader.Consume(1);
                if (auto message = Message::deserializeRaw(reader)) {
                    const auto messageRequiresAck = (currentSeq & kMessageRequiresAckSeqBit) != 0;
                    const auto skipMessage = messageRequiresAck
                        ? !registerSentAck(currentCounter, firstMessageRequiringAck)
                        : additionalMessage && !registerIncomingCounter(currentCounter);
                    if (messageRequiresAck) {
                        firstMessageRequiringAck = false;
                        if (!skipMessage) {
                            newRequiringAckReceived = true;
                        }
                        sendAckPostponed(currentSeq);
                        RTC_LOG(LS_INFO) << (skipMessage ? "Repeated RECV:type" : "Got RECV:type") << type << "#" << currentCounter;
                    }
                    if (!skipMessage) {
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
            if (singleMessagePacket) {
                RTC_LOG(LS_ERROR) << "Single message didn't fill the entire packet.";
                return {};
            }
            if (reader.Length() < 5) {
                RTC_LOG(LS_ERROR) << "Bad remaining data size: " << std::to_string(reader.Length());
                return {};
            }
            const auto success = reader.ReadUInt32(&currentSeq);
            assert(success);
            (void) success;
            currentCounter = CounterFromSeq(currentSeq);
            additionalMessage = true;
        }
        if (!acksToSendSeqs.empty()) {
            if (newRequiringAckReceived) {
                (void) requestSendServiceCallback(0, 0);
            } else if (!sendAcksTimerActive) {
                sendAcksTimerActive = true;
                (void) requestSendServiceCallback(maxDelayBeforeAckResend, kServiceCauseAcks);
            }
        }
        return messages;
    }

    std::optional<uint32_t> SignalingEncryption::computeNextSeq(const bool messageRequiresAck) {
        if (messageRequiresAck && myNotYetAckedMessages.size() >= kNotAckedMessagesLimit) {
            RTC_LOG(LS_ERROR) << "Too many not ACKed messages.";
            return std::nullopt;
        }
        if (counter == kMaxAllowedCounter) {
            RTC_LOG(LS_ERROR) << "Outgoing packet limit reached.";
            return std::nullopt;
        }

        return ++counter | (messageRequiresAck ? kMessageRequiresAckSeqBit : 0);
    }

    bool SignalingEncryption::enoughSpaceInPacket(const rtc::CopyOnWriteBuffer &buffer, const size_t amount) {
        return amount < kMaxSignalingPacketSize && 16 + buffer.size() + amount <= kMaxSignalingPacketSize;
    }

    rtc::CopyOnWriteBuffer SignalingEncryption::SerializeEmptyMessageWithSeq(const uint32_t seq) {
        auto result = rtc::CopyOnWriteBuffer(5);
        const auto bytes = result.MutableData();
        WriteSeq(bytes, seq);
        bytes[4] = kEmptyId;
        return result;
    }

    rtc::CopyOnWriteBuffer SignalingEncryption::SerializeRawMessageWithSeq(const rtc::CopyOnWriteBuffer &message, const uint32_t seq) {
        rtc::ByteBufferWriter writer;
        writer.WriteUInt32(seq);
        writer.WriteUInt8(kCustomId);
        writer.WriteUInt32(static_cast<uint32_t>(message.size()));
        writer.WriteBytes(message.data(), message.size());
        auto result = rtc::CopyOnWriteBuffer();
        result.AppendData(writer.Data(), writer.Length());
        return result;
    }

    void SignalingEncryption::appendMessages(rtc::CopyOnWriteBuffer &buffer) {
        appendAcksToSend(buffer);

        if (myNotYetAckedMessages.empty()) {
            return;
        }
        const auto now = rtc::TimeMillis();
        for (auto &[data, lastSent] : myNotYetAckedMessages) {
            const auto sent = lastSent;
            const auto when = sent ? sent + minDelayBeforeMessageResend : 0;
            assert(data.size() >= 5);
            const auto counter = CounterFromSeq(ReadSeq(data.data()));
            const auto type = static_cast<uint8_t>(data.data()[4]);
            if (when > now) {
                RTC_LOG(LS_INFO)<< "Skip RESEND:type" << type << "#" << counter << " (wait " << when - now << "ms).";
                break;
            }
            if (enoughSpaceInPacket(buffer, data.size())) {
                RTC_LOG(LS_INFO) << "Add RESEND:type" << type << "#" << counter;
                buffer.AppendData(data);
                lastSent = now;
            } else {
                RTC_LOG(LS_INFO) << "Skip RESEND:type" << type << "#" << counter << " (no space, length: " << data.size() << ", already: " << buffer.size() << ")";
                break;
            }
        }

        if (!resendTimerActive) {
            resendTimerActive = true;
            (void) requestSendServiceCallback(
                maxDelayBeforeMessageResend,
                kServiceCauseResend
            );
        }
    }

    void SignalingEncryption::appendAcksToSend(rtc::CopyOnWriteBuffer &buffer) {
        auto i = acksToSendSeqs.begin();
        while (i != acksToSendSeqs.end() && enoughSpaceInPacket(buffer, kAckSerializedSize)) {
            RTC_LOG(LS_INFO) << "Add ACK#" << CounterFromSeq(*i);
            AppendSeq(buffer, *i);
            buffer.AppendData(&kAckId, 1);
            ++i;
        }
        acksToSendSeqs.erase(acksToSendSeqs.begin(), i);
        for (const auto seq : acksToSendSeqs) {
            RTC_LOG(LS_INFO) << "Skip ACK#" << CounterFromSeq(seq) << " (no space, length: " << kAckSerializedSize << ", already: " << buffer.size() << ")";
        }
    }

    bool SignalingEncryption::haveMessages() const {
        return !myNotYetAckedMessages.empty() || !acksToSendSeqs.empty();
    }

    std::optional<bytes::binary> SignalingEncryption::prepareForSendingMessageInternal(rtc::CopyOnWriteBuffer &serialized, uint32_t seq) {
        if (!enoughSpaceInPacket(serialized, 0)) {
            RTC_LOG(LS_ERROR) << "Too large packet: " << std::to_string(serialized.size());
            return std::nullopt;
        }
        const auto notYetAckedCopy = serialized;
        const auto type = static_cast<uint8_t>(serialized.cdata()[4]);
        const auto sendEnqueued = !myNotYetAckedMessages.empty();
        if (sendEnqueued) {
            RTC_LOG(LS_INFO) << "Enqueue SEND:type" << type << "#" << CounterFromSeq(seq);
        } else {
            RTC_LOG(LS_INFO) << "Add SEND:type" << type << "#" << CounterFromSeq(seq);
            appendMessages(serialized);
        }
        myNotYetAckedMessages.push_back({notYetAckedCopy, rtc::TimeMillis()});
        if (!sendEnqueued) {
            return encryptPrepared(serialized);
        }
        for (auto &[data, lastSent] : myNotYetAckedMessages) {
            lastSent = 0;
        }
        return prepareForSendingService(0);
    }

    std::optional<bytes::binary> SignalingEncryption::encrypt(const rtc::CopyOnWriteBuffer &buffer, const bool isRaw) {
        if (isRaw) {
            const auto maybeSeq = computeNextSeq(true);
            if (!maybeSeq) {
                return std::nullopt;
            }
            const auto seq = *maybeSeq;
            auto serialized = SerializeRawMessageWithSeq(buffer, seq);
            return prepareForSendingMessageInternal(serialized, seq);
        }
        const auto seq = ++counter;
        rtc::ByteBufferWriter writer;
        writer.WriteUInt32(seq);
        auto result = rtc::CopyOnWriteBuffer();
        result.AppendData(writer.Data(), writer.Length());
        result.AppendData(buffer);
        return encryptPrepared(result);
    }

    std::vector<rtc::CopyOnWriteBuffer> SignalingEncryption::decrypt(const rtc::CopyOnWriteBuffer &buffer, const bool isRaw) {
        if (buffer.size() < 21 || buffer.size() > kMaxIncomingPacketSize) {
            RTC_LOG(LS_ERROR) << "Bad incoming data size";
            return {};
        }
        const auto x = (_key.isOutgoing ? 8 : 0) + 128;
        const auto key = _key.value->data();
        const auto msgKey = buffer.data();
        const auto encryptedData = msgKey + 16;
        const auto dataSize = buffer.size() - 16;

        auto aesKeyIv = openssl::Aes::PrepareKeyIv(key, msgKey, x);
        auto decryptionBuffer = rtc::Buffer(dataSize);
        openssl::Aes::ProcessCtr(
            bytes::memory_span(encryptedData, dataSize),
            decryptionBuffer.data(),
            aesKeyIv
        );

        if (const auto msgKeyLarge = openssl::Sha256::Concat(
            bytes::memory_span(key + 88 + x, 32),
            bytes::memory_span(decryptionBuffer.data(), decryptionBuffer.size())
        ); ConstTimeIsDifferent(msgKeyLarge.data() + 8, msgKey, 16)) {
            RTC_LOG(LS_ERROR) << "Bad incoming data hash";
            return {};
        }

        const auto incomingSeq = ReadSeq(decryptionBuffer.data());
        if (const auto incomingCounter = CounterFromSeq(incomingSeq); !registerIncomingCounter(incomingCounter)) {
            RTC_LOG(LS_ERROR) << "Already handled packet received." << std::to_string(incomingCounter);
            return {};
        }

        if (isRaw) {
            return processRawPacket(decryptionBuffer, incomingSeq);
        }
        rtc::CopyOnWriteBuffer resultBuffer;
        resultBuffer.AppendData(decryptionBuffer.data() + 4, decryptionBuffer.size() - 4);
        return {resultBuffer};
    }

    void SignalingEncryption::onServiceMessage(const std::function<void(int delayMs, int cause)> &requestSendService) {
        requestSendServiceCallback = requestSendService;
    }

    std::optional<bytes::binary> SignalingEncryption::prepareForSendingService(const int cause) {
        if (cause == kServiceCauseAcks) {
            sendAcksTimerActive = false;
        } else if (cause == kServiceCauseResend) {
            resendTimerActive = false;
        }
        if (!haveMessages()) {
            return std::nullopt;
        }
        const auto seq = computeNextSeq(false);
        if (!seq) {
            return std::nullopt;
        }
        auto serialized = SerializeEmptyMessageWithSeq(*seq);
        if (!enoughSpaceInPacket(serialized, 0)) {
            RTC_LOG(LS_ERROR) << "Failed to serialize empty message";
            return std::nullopt;
        }
        RTC_LOG(LS_INFO) << "SEND:empty#" << CounterFromSeq(*seq);
        appendMessages(serialized);
        return encryptPrepared(serialized);
    }
} // signaling