//
// Created by Laky64 on 04/08/2023.
//

#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "base_reader.hpp"
#include "../exceptions.hpp"

namespace ntgcalls {
    class FileReader final: public BaseReader {
    private:
        std::ifstream source;
        std::string filePath;

    public:
        explicit FileReader(const std::string& path);

        wrtc::binary read(std::int64_t size) final;

        void close() final;

        bool eof() override;
    };
}
