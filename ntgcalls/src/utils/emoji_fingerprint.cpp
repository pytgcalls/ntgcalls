//
// Created by Laky-64 on 21/06/26.
//

#include <ntgcalls/utils/emoji_fingerprint.hpp>

#include <array>
#include <cstdint>

namespace ntgcalls {
    uint64_t EmojiFingerprint::computeIndex(const bytes::const_span part) {
        uint64_t value = 0;
        for (int i = 0; i < kPartSize; ++i) {
            auto byte = static_cast<uint64_t>(std::to_integer<uint8_t>(part[i]));
            if (i == 0) {
                byte &= 0x7F;
            }
            value = value << 8 | byte;
        }
        return value;
    }

    void EmojiFingerprint::appendCodepoint(std::string &out, const uint32_t codepoint) {
        if (codepoint < 0x80) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint < 0x800) {
            out.push_back(static_cast<char>(0xC0 | codepoint >> 6));
            out.push_back(static_cast<char>(0x80 | codepoint & 0x3F));
        } else if (codepoint < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | codepoint >> 12));
            out.push_back(static_cast<char>(0x80 | codepoint >> 6 & 0x3F));
            out.push_back(static_cast<char>(0x80 | codepoint & 0x3F));
        } else {
            out.push_back(static_cast<char>(0xF0 | codepoint >> 18));
            out.push_back(static_cast<char>(0x80 | codepoint >> 12 & 0x3F));
            out.push_back(static_cast<char>(0x80 | codepoint >> 6 & 0x3F));
            out.push_back(static_cast<char>(0x80 | codepoint & 0x3F));
        }
    }

    void EmojiFingerprint::appendEmoji(std::string &out, const int index) {
        uint16_t high = 0;
        for (int i = Offsets[index]; i < Offsets[index + 1]; ++i) {
            const uint16_t unit = Data[i];
            if (unit >= 0xD800 && unit <= 0xDBFF) {
                high = unit;
                continue;
            }
            if (high && unit >= 0xDC00 && unit <= 0xDFFF) {
                appendCodepoint(out, 0x10000 + ((static_cast<uint32_t>(high) - 0xD800) << 10) + (unit - 0xDC00));
                high = 0;
            } else {
                appendCodepoint(out, unit);
            }
        }
    }

    std::string EmojiFingerprint::fromHash(const bytes::const_span hash) {
        if (hash.size() < static_cast<size_t>(kPartSize) * kEmojiInFingerprint) {
            return {};
        }
        std::string result;
        for (int part = 0; part < kEmojiInFingerprint; ++part) {
            const auto index = static_cast<int>(computeIndex(hash.subspan(part * kPartSize, kPartSize)) % kEmojiCount);
            appendEmoji(result, index);
        }
        return result;
    }
} // ntgcalls
