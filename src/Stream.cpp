//
// Created by Laky64 on 03/08/2023.
//

#include "Stream.hpp"

Stream::Stream(std::shared_ptr<BaseReader> audio) {
    audioSrc = std::make_shared<RTCAudioSource>(rtc::Description::Direction::SendOnly);
    audioReader = std::move(audio);
    sampleDuration_us = 1000 * 1000 / 50;
    sampleTime_us = UINT64_MAX - sampleDuration_us + 1;
}

void Stream::start() {
    startTime = getMicroseconds();
    dispatchQueue.dispatch([this]() {
        processData();
    });
}

void Stream::processData() {
    auto currentTime = getMicroseconds();
    auto elapsed = currentTime - startTime;
    if (sampleTime_us > elapsed) {
        auto waitTime = sampleTime_us - elapsed;
        std::this_thread::sleep_for(std::chrono::microseconds(waitTime));
    }

    sampleTime_us += sampleDuration_us;
    audioSrc->sendData(audioReader->read(), sampleTime_us);

    dispatchQueue.dispatch([this]() {
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