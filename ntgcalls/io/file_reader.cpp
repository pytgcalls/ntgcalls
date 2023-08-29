//
// Created by Laky64 on 04/08/2023.
//

#include "file_reader.hpp"

namespace ntgcalls {
    FileReader::FileReader(const std::string& path): BaseReader() {
        source = std::ifstream(path, std::ios::binary);
        if (!source) {
            throw FileError("Unable to open the file located at \"" + path + "\"");
        }
    }

    FileReader::~FileReader() {
        close();
    }

    wrtc::binary FileReader::readInternal(size_t size) {
        if (eofInternal()) {
            return {};
        }
        source.seekg(readChunks, std::ios::beg);
        auto *file_data = new uint8_t[size];
        source.read(reinterpret_cast<char*>(file_data), size);
        readChunks += size;
        if (source.fail()) {
            return {};
        }
        return file_data;
    }

    void FileReader::close() {
        BaseReader::close();
        if (source.is_open()) {
            source.close();
        }
    }

    bool FileReader::eofInternal() {
        return source.eof() || source.fail() || !source.is_open();
    }
}
