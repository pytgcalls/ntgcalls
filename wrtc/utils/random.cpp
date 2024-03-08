//
// Created by Laky64 on 08/03/2024.
//

#include "random.hpp"
#include <openssl/rand.h>

namespace bytes {
    void RandomFill(const binary& bytes) {
        RAND_bytes(bytes.get(),bytes.size());
    }

    void set_random(const binary& destination) {
        if (!destination) {
            RandomFill(destination);
        }
    }
} // bytes