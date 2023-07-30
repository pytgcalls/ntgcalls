//
// Created by Laky64 on 29/07/2023.
//

#ifndef NTGCALLS_MEDIA_DESCRIPTION_H
#define NTGCALLS_MEDIA_DESCRIPTION_H

#include <string>
#include "utils.hpp"

class MediaDescription {
public:
    std::string cname;
    std::string trackId;
    std::string msid;
    std::string mid;
    uint32_t ssrc;

    explicit MediaDescription(uint8_t mid);
};

#endif