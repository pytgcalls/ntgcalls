//
// Created by Laky64 on 07/03/2024.
//

#include "binary.hpp"
#include "encryption.hpp"

namespace bytes {
    binary::binary(const std::string& str): std::shared_ptr<uint8_t[]>(new uint8_t[str.size()]), _s(str.size()) {
#ifndef IS_MACOS
        std::ranges::copy(str, get());
#else
        std::copy(str.begin(), str.end(), get());
#endif
    }

    bool binary::empty() const {
        return _s == 0 || get() == nullptr;
    }

    size_t binary::size() const {
        return _s;
    }

    binary binary::subBytes(const size_t start, const size_t count) const {
        if (start >= _s || count > _s - start) {
            throw std::out_of_range("Invalid subspan parameters");
        }
        return {get() + start, count};
    }

    binary binary::Sha256() const {
        return openssl::Sha256::Digest(*this);
    }

    binary binary::Sha1() const {
       return openssl::Sha1::Digest(*this);
    }

    binary::operator unsigned char*() const {
        return get();
    }

    binary binary::operator+(const int64_t offset) const {
        return subBytes(offset, size() - offset);
    }

    void set_with_const(const binary& destination, const uint8_t value) {
        memset(destination, value, destination.size());
    }

    void copy(const binary& destination, const binary& source) {
        if (destination.size() < source.size()) {
            throw std::out_of_range("Destination size is less than source size");
        }
        memcpy(destination, source, source.size());
    }
} // wrtc