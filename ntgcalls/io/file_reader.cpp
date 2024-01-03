//
// Created by Laky64 on 04/08/2023.
//

#include "file_reader.hpp"

namespace ntgcalls {
    FileReader::FileReader(const std::string& path) {
        source = std::ifstream(path, std::ios::binary);
        if (!source) {
            throw FileError("Unable to open the file located at \"" + path + "\"");
        }
    }

    FileReader::~FileReader() {
        close();
        source.clear();
    }

    wrtc::binary FileReader::readInternal(const int64_t size) {
        if (!source || source.eof() || source.fail() || !source.is_open()) {
            throw EOFError("Reached end of the file");
        }
        source.seekg(readChunks, std::ios::beg);
        auto file_data = std::make_shared<uint8_t[]>(size);
        source.read(reinterpret_cast<char*>(file_data.get()), size);
        readChunks += size;
        if (source.fail()) {
            throw FileError("Error while reading the file");
        }
        return file_data;
    }

    void FileReader::close() {
        BaseReader::close();
        if (source.is_open()) {
            source.close();
        }
    }
}
