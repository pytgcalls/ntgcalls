//
// Created by Laky64 on 04/08/2023.
//

#ifndef NTGCALLS_FILEREADER_HPP
#define NTGCALLS_FILEREADER_HPP


#include <fstream>
#include <string>
#include <vector>
#include "BaseReader.hpp"

class FileReader final: public BaseReader {
private:
    std::ifstream source;
    std::string filePath;

public:
    explicit FileReader(const std::string& path, std::int64_t size = 65536);

    rtc::binary read() final;

    void close() final;
};


#endif
