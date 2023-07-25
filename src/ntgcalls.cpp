//
// Created by iraci on 25/07/2023.
//

#include <iostream>

#include "rtc/rtc.hpp"


int main() {
    try {
        auto test = rtc::PeerConnection();
        std::cout << "test";

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}