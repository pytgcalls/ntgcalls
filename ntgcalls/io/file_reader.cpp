//
// Created by Laky64 on 04/08/2023.
//

#include "file_reader.hpp"

namespace ntgcalls {
    FileReader::FileReader(const std::string& path) {
        source = std::ifstream(path, std::ios::binary);
        if (!source) {
            throw OSError("Unable to open the file located at \"" + path + "\"");
        }
        filePath = path;
    }

    wrtc::binary FileReader::read(std::int64_t size) {
        if (source.eof()) {
            return {};
        }
        source.seekg(readChunks, std::ios::beg);
        auto *file_data = new uint8_t[size];
        source.read(reinterpret_cast<char*>(file_data), size);
        readChunks += size;
        if (source.fail()) {
            throw OSError("Error while reading the file \"" + filePath + "\"");
        }
        return file_data;
    }

    void FileReader::close() {
        source.close();
        readChunks = 0;
        filePath = "";
    }
}
