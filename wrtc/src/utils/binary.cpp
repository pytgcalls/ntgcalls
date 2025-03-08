//
// Created by Laky64 on 07/03/2024.
//

#include <cstring>
#include <wrtc/utils/binary.hpp>

#include <stdexcept>

namespace bytes {
    std::string to_string(const binary& buffer) {
        return {buffer.begin(), buffer.end()};
    }

    void set_with_const(span destination, const byte value) {
        memset(destination.data(), std::to_integer<unsigned char>(value), destination.size());
    }

    void copy(span destination, const const_span source) {
        if (destination.size() < source.size()) {
            throw std::out_of_range("Destination size is less than source size");
        }
        memcpy(destination.data(), source.data(), source.size());
    }
} // wrtc