//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include <cstdint>
#include <string>

namespace wrtc {

    struct RTCServer {
        uint8_t id = 0;
        std::string host;
        uint16_t port = 0;
        std::string login;
        std::string password;
        bool isTurn = false;
        bool isTcp = false;
    };

} // wrtc
