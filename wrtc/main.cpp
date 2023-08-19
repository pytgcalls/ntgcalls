//
// Created by Laky64 on 15/08/2023.
//

#include <iostream>
#include "wrtc.hpp"

int main() {
    std::cout << "TEST" << std::endl;
    auto pc = wrtc::PeerConnection();
    std::cout << "TEST2" << std::endl;
    /*auto test = pc->createOffer(true);
    std::cout << "TEST3" << std::endl;*/
    return 0;
}