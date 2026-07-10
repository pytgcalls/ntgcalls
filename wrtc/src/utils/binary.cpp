//
// Created by Lauren on 07/03/24.
//

#include <cstring>
#include <stdexcept>
#include <wrtc/utils/binary.hpp>

namespace bytes {
    std::string to_string(const binary& buffer) {
        return {buffer.begin(), buffer.end()};
    }

    void set_with_const(span destination, const byte value) {
        std::memset(destination.data(), value, destination.size());
    }

    void copy(span destination, const const_span source) {
        if (destination.size() < source.size()) {
            throw std::out_of_range("Destination size is less than source size");
        }
        std::memcpy(destination.data(), source.data(), source.size());
    }
} // wrtc