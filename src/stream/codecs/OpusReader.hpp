//
// Created by Laky64 on 01/08/2023.
//

#ifndef NTGCALLS_OPUSREADER_HPP
#define NTGCALLS_OPUSREADER_HPP

#include "../FileReader.hpp"

class OpusReader: public FileReader {
    static const uint32_t defaultSamplesPerSecond = 50;

public:
    explicit OpusReader(const std::string& directory, uint32_t samplesPerSecond = OpusReader::defaultSamplesPerSecond);

    void init() override;
};


#endif
