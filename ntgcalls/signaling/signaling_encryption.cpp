//
// Created by Laky64 on 09/03/2024.
//

#include "signaling_encryption.hpp"

#include <rtc_base/copy_on_write_buffer.h>

#include <utility>
#include <rtc_base/logging.h>

#include "wrtc/utils/encryption.hpp"

namespace ntgcalls {
    SignalingEncryption::SignalingEncryption(EncryptionKey key): _key(std::move(key)) {}

    SignalingEncryption::~SignalingEncryption() {
        counter = 0;
        largestIncomingCounters.clear();
    }

    bytes::binary SignalingEncryption::encryptPrepared(const rtc::CopyOnWriteBuffer &buffer) const {
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
            msgKey + 16,
            aesKeyIv
        );
        return encrypted;
    }

    uint32_t SignalingEncryption::ReadSeq(const void* bytes) {
        return rtc::NetworkToHost32(*static_cast<const uint32_t*>(bytes));
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

    rtc::CopyOnWriteBuffer SignalingEncryption::encrypt(const rtc::CopyOnWriteBuffer &buffer) {
        const auto seq = ++counter;

        rtc::ByteBufferWriter writer;
        writer.WriteUInt32(seq);

        auto result = rtc::CopyOnWriteBuffer();
        result.AppendData(writer.Data(), writer.Length());

        result.AppendData(buffer);

        const auto encryptedPacket = encryptPrepared(result);

        rtc::CopyOnWriteBuffer encryptedBuffer;
        encryptedBuffer.AppendData(encryptedPacket.data(), encryptedPacket.size());
        return encryptedBuffer;
    }

    std::optional<rtc::CopyOnWriteBuffer> SignalingEncryption::decrypt(const rtc::CopyOnWriteBuffer &buffer) {
        if (buffer.size() < 21 || buffer.size() > kMaxIncomingPacketSize) {
            return std::nullopt;
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
            return std::nullopt;
        }

        const auto incomingSeq = ReadSeq(decryptionBuffer.data());
        if (const auto incomingCounter = CounterFromSeq(incomingSeq); !registerIncomingCounter(incomingCounter)) {
            RTC_LOG(LS_ERROR) << "Already handled packet received." << std::to_string(incomingCounter);
            return std::nullopt;
        }

        rtc::CopyOnWriteBuffer resultBuffer;
        resultBuffer.AppendData(decryptionBuffer.data() + 4, decryptionBuffer.size() - 4);
        return resultBuffer;
    }
} // ntgcalls