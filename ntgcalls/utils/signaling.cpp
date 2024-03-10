//
// Created by Laky64 on 09/03/2024.
//

#include "signaling.hpp"

#include <iostream>
#include <rtc_base/copy_on_write_buffer.h>
#include "wrtc/utils/encryption.hpp"

namespace ntgcalls {
    Signaling::Signaling(const bool isOutGoing, bytes::binary key): isOutGoing(isOutGoing), key(std::move(key)) {}

    bytes::binary Signaling::encryptPrepared(const bytes::binary& data) const {
        const auto encrypted = bytes::binary(16 + data.size());
        const auto x = (isOutGoing ? 0 : 8) + 128;
        const auto msgKeyLarge = openssl::Sha256::Concat(
            key + 88 + x,
            data
        );
        memcpy(encrypted, msgKeyLarge + 8, 16);
        const auto aesKeyIv = openssl::Aes::PrepareKeyIv(key, encrypted, x);
        openssl::Aes::ProcessCtr(
            data,
            encrypted + 16,
            aesKeyIv
        );
        return encrypted;
    }

    uint32_t Signaling::ReadSeq(const void* bytes) {
        return rtc::NetworkToHost32(*static_cast<const uint32_t*>(bytes));
    }

    uint32_t Signaling::CounterFromSeq(const uint32_t seq) {
        return seq & ~kSingleMessagePacketSeqBit & ~kMessageRequiresAckSeqBit;
    }

    // ReSharper disable once CppDFAConstantParameter
    bool Signaling::ConstTimeIsDifferent(const void *a, const void *b, const size_t size) {
        auto ca = static_cast<const char*>(a);
        auto cb = static_cast<const char*>(b);
        volatile auto different = false;
        for (const auto ce = ca + size; ca != ce; ++ca, ++cb) {
            different = different | (*ca != *cb);
        }
        return different;
    }

    bool Signaling::registerIncomingCounter(const uint32_t incomingCounter) {
        auto &list = _largestIncomingCounters;
        const auto position = std::ranges::lower_bound(list, incomingCounter);
        const auto largest = list.empty() ? 0 : list.back();
        if (position != list.end() && *position == incomingCounter) {
            // The packet is in the list already.
            return false;
        }
        if (incomingCounter + kKeepIncomingCountersCount <= largest) {
            // The packet is too old.
            return false;
        }
        const auto eraseTill = std::ranges::find_if(list, [&](const uint32_t counter) {
            return counter + kKeepIncomingCountersCount > incomingCounter;
        });
        const auto eraseCount = eraseTill - list.begin();
        const auto positionIndex = (position - list.begin()) - eraseCount;
        list.erase(list.begin(), eraseTill);

        assert(positionIndex >= 0 && positionIndex <= list.size());
        list.insert(list.begin() + positionIndex, incomingCounter);
        return true;
    }

    bytes::binary Signaling::encrypt(const bytes::binary& buffer) {
        const auto seq = ++counter;
        rtc::ByteBufferWriter writer;
        writer.WriteUInt32(seq);
        auto result = rtc::CopyOnWriteBuffer();
        result.AppendData(writer.Data(), writer.Length());
        result.AppendData(buffer.get(), buffer.size());
        return encryptPrepared(bytes::binary(result.data(), result.size()));
    }

    bytes::binary Signaling::decrypt(const bytes::binary& buffer) {
        if (buffer.size() < 21 || buffer.size() > kMaxIncomingPacketSize) {
            std::cout << "Invalid packet size" << std::endl;
            return nullptr;
        }
        const auto x = (isOutGoing ? 0 : 8) + 128;
        const auto encryptedData = buffer + 16;
        const auto aesKeyIv = openssl::Aes::PrepareKeyIv(key, buffer, x);
        const auto decrypted = bytes::binary(buffer.size() - 16);
        openssl::Aes::ProcessCtr(
            encryptedData,
            decrypted,
            aesKeyIv
        );
        if (const auto msgKeyLarge = openssl::Sha256::Concat(
            key + 88 + x,
            decrypted
        ); ConstTimeIsDifferent(msgKeyLarge + 8, buffer, 16)) {
            std::cout << "Invalid message key" << std::endl;
            return nullptr;
        }

        const auto incomingSeq = ReadSeq(decrypted);
        if (const auto incomingCounter = CounterFromSeq(incomingSeq); !registerIncomingCounter(incomingCounter)) {
            // We've received that packet already.
            std::cout << "We've received that packet already." << std::endl;
            return nullptr;
        }
        return decrypted + 4;
    }
} // ntgcalls