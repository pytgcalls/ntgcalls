//
// Created by Lauren on 08/03/24.
//

#include <wrtc/utils/random.hpp>
#include <openssl/rand.h>

namespace bytes {
    void random_fill(span data) {
        RAND_bytes(data.data(), data.size());
    }

    void set_random(const span destination) {
        if (!destination.empty()) {
            random_fill(destination);
        }
    }
} // bytes