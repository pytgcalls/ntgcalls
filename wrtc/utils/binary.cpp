//
// Created by Laky64 on 07/03/2024.
//

#include "binary.hpp"

namespace bytes {
    bool binary::empty() const {
        return _s == 0 || get() == nullptr;
    }

    size_t binary::size() const {
        return _s;
    }

    binary binary::subspan(const size_t start, const size_t count) const {
        if (start >= _s || count > _s - start) {
            throw std::out_of_range("Invalid subspan parameters");
        }
        return {get() + start, count};
    }

    binary binary::Sha256() const {
        auto bytes = binary(SHA256_DIGEST_LENGTH);
        SHA256(get(), size(), bytes.get());
        return bytes;
    }

    binary binary::Sha1() const {
        auto bytes = binary(SHA_DIGEST_LENGTH);
        SHA1(get(), size(), bytes.get());
        return bytes;
    }

    void set_with_const(const binary& destination, const uint8_t value) {
        memset(destination.get(), value, destination.size());
    }

    void copy(const binary& destination, const binary& source) {
        if (destination.size() < source.size()) {
            throw std::out_of_range("Destination size is less than source size");
        }
        memcpy(destination.get(), source.get(), source.size());
    }
} // wrtc