//
// Created by Laky64 on 02/08/2023.
//

#ifndef NTGCALLS_H264READER_HPP
#define NTGCALLS_H264READER_HPP


#include "../FileReader.hpp"

class H264Reader: public FileReader {
    std::optional<std::vector<std::byte>> previousUnitType5 = std::nullopt;
    std::optional<std::vector<std::byte>> previousUnitType7 = std::nullopt;
    std::optional<std::vector<std::byte>> previousUnitType8 = std::nullopt;

public:
    H264Reader(const std::string& directory, uint32_t fps);

    void loadNextSample() override;

    void init() override;

    std::vector<std::byte> initialNALUS();
};


#endif
