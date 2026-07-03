//
// Created by Lauren on 04/08/23.
//

#pragma once

#include <fstream>
#include <string>
#include <ntgcalls/io/threaded_reader.hpp>

namespace ntgcalls::io {
    class FileReader final: public ThreadedReader {
        std::ifstream source_;

    public:
        explicit FileReader(const std::string& path, media::BaseSink *sink);

        ~FileReader() override;

        void open() override;
    };
} // ntgcalls::io
