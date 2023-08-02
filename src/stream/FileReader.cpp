//
// Created by Laky64 on 01/08/2023.
//

#include "FileReader.hpp"

FileReader::FileReader(const std::string &directory, const std::string &extension, uint32_t samplesPerSecond, bool isVideo) : StreamSource(isVideo) {
    this->directory = directory;
    this->extension = extension;
    this->sampleDuration_us = 1000 * 1000 / samplesPerSecond;
}

FileReader::~FileReader() {
    cleanup();
}

void FileReader::start() {
    sampleTime_us = uint64_t(std::numeric_limits<uint64_t>::max()) - sampleDuration_us + 1;
    loadNextSample();
}

void FileReader::stop() {
    sample = {};
    sampleTime_us = 0;
    counter = -1;
}


void FileReader::loadNextSample() {
    std::string frame_id = std::to_string(++counter);

    std::string url = directory + "/sample-" + frame_id + extension;
    std::ifstream source(url, std::ios_base::binary);
    if (!source) {
        sample = {};
        return;
    }

    std::vector<uint8_t> fileContents((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    sample = *reinterpret_cast<std::vector<std::byte> *>(&fileContents);
    sampleTime_us += sampleDuration_us;
}

rtc::binary FileReader::getSample() {
    return sample;
}

uint64_t FileReader::getSampleTime_us() {
    return sampleTime_us;
}

uint64_t FileReader::getSampleDuration_us() {
    return sampleDuration_us;
}

void FileReader::cleanup() {
    stop();
}
