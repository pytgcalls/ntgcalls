//
// Created by Laky64 on 03/08/2023.
//

#include "Stream.hpp"

Stream::Stream() {
    sampleDuration_us = 1000 * 1000 / 50; // 50 samples per seconds
    audioSrc = std::make_shared<RTCAudioSource>();
}

void Stream::start() {
    if (isRunning) {
        return;
    }
    isRunning = true;
    sampleTime_us = UINT64_MAX - sampleDuration_us + 1;
    std::cout << "[LOADING SAMPLES...]" << std::endl;
    loadNextSample();
    std::cout << "[SAMPLE LOADED]" << std::endl;
    dispatchQueue.dispatch([this]() {
        this->sendSample();
    });
}

void Stream::loadNextSample() {
    std::string frame_id = std::to_string(++counter);
    std::string dir = "C:/Users/iraci/PycharmProjects/NativeTgCalls/tools/samples/opus/";
    std::string url = dir + "sample-" + frame_id + ".opus";
    std::ifstream source(url, std::ios_base::binary);
    if (!source) {
        if (counter > 0) {
            counter = -1;
            loadNextSample();
            return;
        }
        sample = {};
        return;
    }

    std::vector<uint8_t> fileContents((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    sample = *reinterpret_cast<std::vector<std::byte> *>(&fileContents);
    sampleTime_us += sampleDuration_us;
}

void Stream::unsafePrepareForSample() {
    uint64_t nextTime = sampleTime_us;

    auto currentTime = getMicroseconds();

    auto elapsed = currentTime - startTime;

    if (nextTime > elapsed) {
        auto waitTime = nextTime - elapsed;
        mutex.unlock();
        usleep(static_cast<int64_t>(waitTime));
        mutex.lock();
    }
}

void Stream::sendSample() {
    std::lock_guard lock(mutex);
    if (!isRunning) {
        return;
    }
    unsafePrepareForSample();
    audioSrc->sendData(sample, sampleTime_us);
    loadNextSample();
    dispatchQueue.dispatch([this]() {
        this->sendSample();
    });
}


void Stream::addTracks(const std::shared_ptr<rtc::PeerConnection>& pc) {
    audioSrc->addTrack(pc);
    audioSrc->onOpen([this]() {
        std::cout << "[STREAM OPENED]" << std::endl;
        start();
    });
}