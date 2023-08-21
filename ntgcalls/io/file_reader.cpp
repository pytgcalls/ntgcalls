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
        filePath = path;
    }

    wrtc::binary FileReader::read(std::int64_t size) {
        if (source.eof() || source.fail()) {
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
        source.close();
        readChunks = 0;
        filePath = "";
    }

    bool FileReader::eof() {
        return source.eof() || source.fail();
    }
}
