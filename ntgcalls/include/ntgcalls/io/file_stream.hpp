//
// Created by Lauren on 04/08/23.
//

#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <ntgcalls/io/base_stream.hpp>

namespace ntgcalls::io {
    class FileStream: public BaseStream {
        std::mutex mutex_;
        std::fstream file_;
        int64_t read_bytes_ = 0;

    public:
        FileStream(const std::string& path, Mode mode);

        ~FileStream() override;

        bytes::unique_binary read(int64_t size) override;

        void write(const bytes::unique_binary& data, size_t size) override;

        void close() override;
    };
} // ntgcalls::io
