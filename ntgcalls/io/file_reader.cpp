//
// Created by Laky64 on 04/08/2023.
//

#include "file_reader.hpp"

namespace ntgcalls {
    FileReader::FileReader(const std::string& path, const int64_t bufferSize, const bool noLatecy): BaseReader(bufferSize, noLatecy) {
        source = std::ifstream(path, std::ios::binary);
        if (!source) {
            RTC_LOG(LS_ERROR) << "Unable to open the file located at \"" << path << "\"";
            throw FileError("Unable to open the file located at \"" + path + "\"");
        }
    }

    FileReader::~FileReader() {
        close();
        source.clear();
    }

    bytes::shared_binary FileReader::readInternal(const int64_t size) {
        if (!source || source.eof() || source.fail() || !source.is_open()) {
            RTC_LOG(LS_WARNING) << "Reached end of the file";
            throw EOFError("Reached end of the file");
        }
        source.seekg(readChunks, std::ios::beg);
        auto file_data = bytes::make_shared_binary(size);
        source.read(reinterpret_cast<char*>(file_data.get()), size);
        readChunks += size;
        if (source.fail()) {
            RTC_LOG(LS_ERROR) << "Error while reading the file";
            throw FileError("Error while reading the file");
        }
        return file_data;
    }

    void FileReader::close() {
        BaseReader::close();
        if (source.is_open()) {
            source.close();
        }
        RTC_LOG(LS_VERBOSE) << "FileReader closed";
    }
}
