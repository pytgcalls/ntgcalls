//
// Created by Lauren on 29/03/24.
//

#pragma once
#include <cstdint>
#include <string>

namespace wrtc::models {

    struct RTCServer {
        uint8_t id = 0;
        std::string host;
        uint16_t port = 0;
        std::string login;
        std::string password;
        bool is_turn = false;
        bool is_tcp = false;
    };

} // wrtc::models
