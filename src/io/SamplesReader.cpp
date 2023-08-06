//
// Created by Laky64 on 06/08/2023.
//

#include "SamplesReader.hpp"

SamplesReader::SamplesReader(const std::string &path, const std::string &extension) {
    filePath = path;
    ext = extension;
}

rtc::binary SamplesReader::read() {
    std::string frame_id = std::to_string(++counter);

    std::string url = filePath + "/sample-" + frame_id + ext;
    std::ifstream source(url, std::ios_base::binary);
    if (!source) {
        return {};
    }
    std::vector<uint8_t> fileContents((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    return *reinterpret_cast<std::vector<std::byte> *>(&fileContents);
}