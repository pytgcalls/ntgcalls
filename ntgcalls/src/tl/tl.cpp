//
// Created by Lauren on 07/06/26.
//

#include <algorithm>
#include <ntgcalls/tl/tl.hpp>

namespace ntgcalls::tl {
    void TlWriter::store_u_int32(const uint32_t value) {
        buffer_.push_back(static_cast<bytes::byte>(value & 0xff));
        buffer_.push_back(static_cast<bytes::byte>(value >> 8 & 0xff));
        buffer_.push_back(static_cast<bytes::byte>(value >> 16 & 0xff));
        buffer_.push_back(static_cast<bytes::byte>(value >> 24 & 0xff));
    }

    void TlWriter::store_int32(const int32_t value) {
        store_u_int32(static_cast<uint32_t>(value));
    }

    void TlWriter::store_int64(const int64_t value) {
        const auto raw = static_cast<uint64_t>(value);
        for (int i = 0; i < 8; ++i) {
            buffer_.push_back(static_cast<bytes::byte>(raw >> i * 8 & 0xff));
        }
    }

    void TlWriter::store_int256(const bytes::array<32>& value) {
        buffer_.insert(buffer_.end(), value.begin(), value.end());
    }

    void TlWriter::store_int512(const bytes::array<64>& value) {
        buffer_.insert(buffer_.end(), value.begin(), value.end());
    }

    void TlWriter::store_raw(const bytes::const_span value) {
        const auto data = value.data();
        buffer_.insert(buffer_.end(), data, data + value.size());
    }

    void TlWriter::store_bytes(const bytes::const_span value) {
        const auto size = value.size();
        size_t prefixed;
        if (size < 254) {
            buffer_.push_back(static_cast<bytes::byte>(size));
            prefixed = size + 1;
        } else {
            buffer_.push_back(254);
            buffer_.push_back(static_cast<bytes::byte>(size & 0xff));
            buffer_.push_back(static_cast<bytes::byte>(size >> 8 & 0xff));
            buffer_.push_back(static_cast<bytes::byte>(size >> 16 & 0xff));
            prefixed = size + 4;
        }
        store_raw(value);
        while (prefixed % 4 != 0) {
            buffer_.push_back(0);
            ++prefixed;
        }
    }

    void TlWriter::store_string(const std::string& value) {
        store_bytes(bytes::view(value));
    }

    void TlWriter::store_vector_size(const uint32_t size) {
        store_u_int32(size);
    }

    bytes::binary TlWriter::result() {
        return std::move(buffer_);
    }

    TlReader::TlReader(const bytes::const_span data):
    ptr_(data.data()), end_(ptr_ + data.size()) {}

    uint32_t TlReader::fetch_uint32() {
        if (failed_ || end_ - ptr_ < 4) {
            set_error();
            return 0;
        }
        const auto value = static_cast<uint32_t>(ptr_[0]) |
                           static_cast<uint32_t>(ptr_[1]) << 8 |
                           static_cast<uint32_t>(ptr_[2]) << 16 |
                           static_cast<uint32_t>(ptr_[3]) << 24;
        ptr_ += 4;
        return value;
    }

    int32_t TlReader::fetch_int32() {
        return static_cast<int32_t>(fetch_uint32());
    }

    int64_t TlReader::fetch_int64() {
        if (failed_ || end_ - ptr_ < 8) {
            set_error();
            return 0;
        }
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<uint64_t>(ptr_[i]) << i * 8;
        }
        ptr_ += 8;
        return static_cast<int64_t>(value);
    }

    bytes::array<32> TlReader::fetch_int256() {
        bytes::array<32> value{};
        if (failed_ || end_ - ptr_ < 32) {
            set_error();
            return value;
        }
        std::copy_n(ptr_, 32, value.begin());
        ptr_ += 32;
        return value;
    }

    bytes::array<64> TlReader::fetch_int512() {
        bytes::array<64> value{};
        if (failed_ || end_ - ptr_ < 64) {
            set_error();
            return value;
        }
        std::copy_n(ptr_, 64, value.begin());
        ptr_ += 64;
        return value;
    }

    bytes::binary TlReader::fetch_raw(const size_t size) {
        if (failed_ || static_cast<size_t>(end_ - ptr_) < size) {
            set_error();
            return {};
        }
        bytes::binary value(ptr_, ptr_ + size);
        ptr_ += size;
        return value;
    }

    bytes::binary TlReader::fetch_bytes() {
        if (failed_ || ptr_ >= end_) {
            set_error();
            return {};
        }
        size_t size;
        size_t prefixed;
        if (const auto first = *ptr_++; first < 254) {
            size = first;
            prefixed = size + 1;
        } else if (first == 254) {
            if (end_ - ptr_ < 3) {
                set_error();
                return {};
            }
            size = static_cast<size_t>(ptr_[0]) | static_cast<size_t>(ptr_[1]) << 8 | static_cast<size_t>(ptr_[2]) << 16;
            ptr_ += 3;
            prefixed = size + 4;
        } else {
            set_error();
            return {};
        }
        auto value = fetch_raw(size);
        while (prefixed % 4 != 0) {
            if (failed_ || ptr_ >= end_) {
                set_error();
                return {};
            }
            ++ptr_;
            ++prefixed;
        }
        return value;
    }

    std::string TlReader::fetch_string() {
        const auto raw = fetch_bytes();
        return {reinterpret_cast<const char*>(raw.data()), raw.size()};
    }

    uint32_t TlReader::fetch_vector_size() {
        return fetch_uint32();
    }

    bool TlReader::ok() const {
        return !failed_;
    }

    void TlReader::set_error() {
        failed_ = true;
    }

    bool TlReader::finish() const {
        return !failed_ && ptr_ == end_;
    }
} // ntgcalls::e2e
