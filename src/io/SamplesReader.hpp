//
// Created by Laky64 on 06/08/2023.
//

#ifndef NTGCALLS_SAMPLESREADER_HPP
#define NTGCALLS_SAMPLESREADER_HPP


#include "BaseReader.hpp"
#include <fstream>

class SamplesReader final: public BaseReader {
private:
    uint32_t counter = -1;
    std::string filePath, ext;

public:
    explicit SamplesReader(const std::string& path, const std::string &extension);

    rtc::binary read() final;
};


#endif //NTGCALLS_SAMPLESREADER_HPP
