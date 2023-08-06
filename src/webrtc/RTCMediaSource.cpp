//
// Created by Laky64 on 03/08/2023.
//

#include "RTCMediaSource.hpp"

#include <utility>

RTCMediaSource::RTCMediaSource(MediaStreamTrack::Type type) {
    codec = type;
}

void RTCMediaSource::addTrack(const std::shared_ptr<rtc::PeerConnection> &pc) {
    track = std::make_shared<MediaStreamTrack>(codec, pc);
}

void RTCMediaSource::onOpen(const std::function<void()> &callback) {
    if (track != nullptr) {
        track -> onOpen(callback);
    } else {
        throw std::runtime_error("No tracks found");
    }
}

void RTCMediaSource::sendData(const rtc::binary& samples, uint64_t sampleTime) {
    if (track != nullptr) {
        track -> sendData(samples, sampleTime);
    } else {
        throw std::runtime_error("No tracks found");
    }
}
