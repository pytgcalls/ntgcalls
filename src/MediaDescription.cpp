#include "MediaDescription.hpp"

MediaDescription::MediaDescription(uint8_t id) {
    cname = generateUniqueId(16);
    msid = "-";
    trackId = generateUniqueId(8) + "-" +
            generateUniqueId(4) + "-" +
            generateUniqueId(4) + "-" +
            generateUniqueId(12);
    mid = std::to_string(id);
    ssrc = generateSSRC();
}