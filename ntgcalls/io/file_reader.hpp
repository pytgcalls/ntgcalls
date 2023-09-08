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

        wrtc::binary readInternal(size_t size) final;

    public:
        FileReader(const std::string& path);

        ~FileReader() override;

        void close() final;
    };
}
