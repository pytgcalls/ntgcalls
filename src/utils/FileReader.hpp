//
// Created by Laky64 on 04/08/2023.
//

#ifndef NTGCALLS_FILEREADER_HPP
#define NTGCALLS_FILEREADER_HPP


#include <fstream>
#include <string>
#include <vector>
#include "rtc/rtc.hpp"

class FileReader {
private:
    std::ifstream source;
    int64_t chunkSize, readChunks;
    std::string filePath;

public:
    explicit FileReader(const std::string& path, std::int64_t size = 65536);

    ~FileReader();

    rtc::binary read();

    void close();
};


#endif
