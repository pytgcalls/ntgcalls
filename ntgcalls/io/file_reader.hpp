//
// Created by Laky64 on 04/08/2023.
//

#pragma once

#include <fstream>
#include <string>

#include "base_reader.hpp"
#include "../exceptions.hpp"

namespace ntgcalls {
    class FileReader final: public BaseReader {
        std::ifstream source;

        wrtc::binary readInternal(int64_t size) override;

    public:
        explicit FileReader(const std::string& path);

        ~FileReader() override;

        void close() override;
    };
}
