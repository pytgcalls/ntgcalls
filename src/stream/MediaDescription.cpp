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

void MediaDescription::attachMedia(const rtc::Description::Media& media) {
    mediaDesc = media;
}

rtc::Description::Media MediaDescription::getMedia() {
    if (!mediaDesc.has_value()) {
        throw std::runtime_error("Media description not found");
    }
    return mediaDesc.value();
}
