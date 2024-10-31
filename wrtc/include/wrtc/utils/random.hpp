//
// Created by Laky64 on 08/03/2024.
//

#pragma once

#include <wrtc/utils/binary.hpp>

namespace bytes {
    void RandomFill(span data);

    void set_random(span destination);
} // bytes
