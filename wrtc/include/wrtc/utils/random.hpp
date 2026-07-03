//
// Created by Lauren on 08/03/24.
//

#pragma once

#include <wrtc/utils/binary.hpp>

namespace bytes {
    void random_fill(span data);

    void set_random(span destination);
} // bytes
