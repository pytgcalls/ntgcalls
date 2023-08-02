//
// Created by Laky64 on 29/07/2023.
//

#ifndef NTGCALLS_MEDIA_DESCRIPTION_HPP
#define NTGCALLS_MEDIA_DESCRIPTION_HPP

#include <string>
#include "../utils.hpp"

class MediaDescription {
private:
    std::optional<rtc::Description::Media> mediaDesc;

public:
    std::string cname;
    std::string trackId;
    std::string msid;
    std::string mid;
    uint32_t ssrc;

    explicit MediaDescription(uint8_t mid);

    void attachMedia(const rtc::Description::Media& media);

    rtc::Description::Media getMedia();
};

#endif