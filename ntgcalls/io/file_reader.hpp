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

        bytes::shared_binary readInternal(int64_t size) override;

    public:
        explicit FileReader(const std::string& path, int64_t bufferSize, bool noLatecy);

        ~FileReader() override;

        void close() override;
    };
}
