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
} // wrtc