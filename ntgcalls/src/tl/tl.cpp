//
// Created by Laky64 on 07/06/26.
//

#include <algorithm>
#include <ntgcalls/tl/tl.hpp>

namespace telegram {
    void TlWriter::storeUInt32(const uint32_t value) {
        buffer.push_back(static_cast<uint8_t>(value & 0xff));
        buffer.push_back(static_cast<uint8_t>(value >> 8 & 0xff));
        buffer.push_back(static_cast<uint8_t>(value >> 16 & 0xff));
        buffer.push_back(static_cast<uint8_t>(value >> 24 & 0xff));
    }

    void TlWriter::storeInt32(const int32_t value) {
        storeUInt32(static_cast<uint32_t>(value));
    }

    void TlWriter::storeInt64(const int64_t value) {
        const auto raw = static_cast<uint64_t>(value);
        for (int i = 0; i < 8; ++i) {
            buffer.push_back(static_cast<uint8_t>(raw >> i * 8 & 0xff));
        }
    }

    void TlWriter::storeInt256(const std::array<uint8_t, 32>& value) {
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    void TlWriter::storeInt512(const std::array<uint8_t, 64>& value) {
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    void TlWriter::storeRaw(const bytes::const_span value) {
        const auto data = reinterpret_cast<const uint8_t*>(value.data());
        buffer.insert(buffer.end(), data, data + value.size());
    }

    void TlWriter::storeBytes(const bytes::const_span value) {
        const auto size = value.size();
        size_t prefixed;
        if (size < 254) {
            buffer.push_back(static_cast<uint8_t>(size));
            prefixed = size + 1;
        } else {
            buffer.push_back(254);
            buffer.push_back(static_cast<uint8_t>(size & 0xff));
            buffer.push_back(static_cast<uint8_t>(size >> 8 & 0xff));
            buffer.push_back(static_cast<uint8_t>(size >> 16 & 0xff));
            prefixed = size + 4;
        }
        storeRaw(value);
        while (prefixed % 4 != 0) {
            buffer.push_back(0);
            ++prefixed;
        }
    }

    void TlWriter::storeString(const std::string& value) {
        storeBytes(bytes::view(value));
    }

    void TlWriter::storeVectorSize(const uint32_t size) {
        storeUInt32(size);
    }

    bytes::binary TlWriter::result() {
        return std::move(buffer);
    }

    TlReader::TlReader(const bytes::const_span data):
        ptr(reinterpret_cast<const uint8_t*>(data.data())), end(ptr + data.size()) {}

    uint32_t TlReader::fetchUInt32() {
        if (failed || end - ptr < 4) {
            setError();
            return 0;
        }
        const auto value = static_cast<uint32_t>(ptr[0]) |
            static_cast<uint32_t>(ptr[1]) << 8 |
            static_cast<uint32_t>(ptr[2]) << 16 |
            static_cast<uint32_t>(ptr[3]) << 24;
        ptr += 4;
        return value;
    }

    int32_t TlReader::fetchInt32() {
        return static_cast<int32_t>(fetchUInt32());
    }

    int64_t TlReader::fetchInt64() {
        if (failed || end - ptr < 8) {
            setError();
            return 0;
        }
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<uint64_t>(ptr[i]) << i * 8;
        }
        ptr += 8;
        return static_cast<int64_t>(value);
    }

    std::array<uint8_t, 32> TlReader::fetchInt256() {
        std::array<uint8_t, 32> value{};
        if (failed || end - ptr < 32) {
            setError();
            return value;
        }
        std::copy_n(ptr, 32, value.begin());
        ptr += 32;
        return value;
    }

    std::array<uint8_t, 64> TlReader::fetchInt512() {
        std::array<uint8_t, 64> value{};
        if (failed || end - ptr < 64) {
            setError();
            return value;
        }
        std::copy_n(ptr, 64, value.begin());
        ptr += 64;
        return value;
    }

    bytes::binary TlReader::fetchRaw(const size_t size) {
        if (failed || static_cast<size_t>(end - ptr) < size) {
            setError();
            return {};
        }
        bytes::binary value(ptr, ptr + size);
        ptr += size;
        return value;
    }

    bytes::binary TlReader::fetchBytes() {
        if (failed || ptr >= end) {
            setError();
            return {};
        }
        size_t size;
        size_t prefixed;
        if (const auto first = *ptr++; first < 254) {
            size = first;
            prefixed = size + 1;
        } else if (first == 254) {
            if (end - ptr < 3) {
                setError();
                return {};
            }
            size = static_cast<size_t>(ptr[0]) | static_cast<size_t>(ptr[1]) << 8 | static_cast<size_t>(ptr[2]) << 16;
            ptr += 3;
            prefixed = size + 4;
        } else {
            setError();
            return {};
        }
        auto value = fetchRaw(size);
        while (prefixed % 4 != 0) {
            if (failed || ptr >= end) {
                setError();
                return {};
            }
            ++ptr;
            ++prefixed;
        }
        return value;
    }

    std::string TlReader::fetchString() {
        const auto raw = fetchBytes();
        return {reinterpret_cast<const char*>(raw.data()), raw.size()};
    }

    uint32_t TlReader::fetchVectorSize() {
        return fetchUInt32();
    }

    bool TlReader::ok() const {
        return !failed;
    }

    void TlReader::setError() {
        failed = true;
    }

    bool TlReader::finish() const {
        return !failed && ptr == end;
    }
} // ntgcalls::e2e
