//
// Created by Laky64 on 04/08/2023.
//

#include "FileReader.hpp"
#include "rtc/rtc.hpp"

FileReader::FileReader(const std::string& path, std::int64_t size) {
    source = std::ifstream(path, std::ios::binary);
    if (!source) {
        throw std::invalid_argument("Unable to open the file located at \"" + path + "\"");
    }
    chunkSize = size;
    filePath = path;
}

rtc::binary FileReader::read() {
    if (source.eof()) {
        return {};
    }
    source.seekg(readChunks, std::ios::beg);
    std::vector<uint8_t> file_data(chunkSize);
    source.read(reinterpret_cast<char*>(file_data.data()), chunkSize);
    readChunks += chunkSize;
    if (source.fail()) {
        throw std::runtime_error("Error while reading the file \"" + filePath + "\"");
    }
    return *reinterpret_cast<std::vector<std::byte> *>(&file_data);
}

void FileReader::close() {
    source.close();
    chunkSize = 0;
    readChunks = 0;
    filePath = "";
}