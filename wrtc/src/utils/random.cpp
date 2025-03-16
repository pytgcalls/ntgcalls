//
// Created by Laky64 on 08/03/2024.
//

#include <wrtc/utils/random.hpp>
#include <openssl/rand.h>

namespace bytes {
    void RandomFill(span data) {
        RAND_bytes(reinterpret_cast<unsigned char*>(data.data()), data.size());
    }

    void set_random(const span destination) {
        if (!destination.empty()) {
            RandomFill(destination);
        }
    }
} // bytes