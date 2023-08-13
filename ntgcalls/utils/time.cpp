//
// Created by Laky64 on 03/08/2023.
//

#include "time.hpp"

int64_t getMilliseconds() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return milliseconds;
}

uint64_t getMicroseconds() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    int64_t microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    return microseconds;
}