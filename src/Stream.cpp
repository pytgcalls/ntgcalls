//
// Created by Laky64 on 03/08/2023.
//

#include "Stream.hpp"

Stream::Stream(std::shared_ptr<BaseReader> audio) {
    audioSrc = std::make_shared<RTCAudioSource>();
    audioReader = std::move(audio);
}

void Stream::start() {
    dispatchQueue.dispatch([this]() {
        processData();
    });
}

void Stream::processData() {
    audioSrc->sendData(audioReader->read());
    dispatchQueue.dispatch([this]() {
        usleep(10 * 1000);
        processData();
    });
}


void Stream::addTracks(const std::shared_ptr<rtc::PeerConnection>& pc) {
    audioSrc->addTrack(pc);
    audioSrc->onOpen([this]() {
        std::cout << "[STREAM OPENED]" << std::endl;
        start();
    });
}