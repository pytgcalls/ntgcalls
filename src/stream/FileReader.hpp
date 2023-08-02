//
// Created by Laky64 on 01/08/2023.
//

#ifndef NTGCALLS_FILEREADER_HPP
#define NTGCALLS_FILEREADER_HPP

#include <iostream>
#include "rtc/rtc.hpp"
#include "Stream.hpp"
#include <fstream>

#ifdef max
#undef max
#endif

class FileReader: public StreamSource {
private:
    std::string directory;
    std::string extension;
    uint64_t sampleDuration_us;
    uint64_t sampleTime_us = 0;
    uint32_t counter = -1;

    void cleanup();

protected:
    rtc::binary sample = {};

public:
    FileReader(const std::string &directory, const std::string &extension,
               uint32_t samplesPerSecond, bool isVideo);
    virtual ~FileReader();
    void start() override;
    void stop() override;
    void loadNextSample() override;

    rtc::binary getSample() override;

    uint64_t getSampleTime_us() override;

    uint64_t getSampleDuration_us() override;
};


#endif
