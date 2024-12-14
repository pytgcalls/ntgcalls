//
// Created by Laky64 on 04/08/2023.
//

#include <ntgcalls/io/file_reader.hpp>

namespace ntgcalls {
    FileReader::FileReader(const std::string& path, BaseSink *sink): BaseIO(sink), ThreadedReader(sink) {
        source = std::ifstream(path, std::ios::binary);
        if (!source) {
            RTC_LOG(LS_ERROR) << "Unable to open the file located at \"" << path << "\"";
            throw FileError("Unable to open the file located at \"" + path + "\"");
        }
    }

    FileReader::~FileReader() {
        close();
        RTC_LOG(LS_VERBOSE) << "ThreadedReader closed";
        if (source.is_open()) {
            source.close();
        }
        source.clear();
        RTC_LOG(LS_VERBOSE) << "FileReader closed";
    }

    void FileReader::open() {
        run([this](const int64_t size) {
            if (!source || source.eof() || source.fail() || !source.is_open()) {
                RTC_LOG(LS_WARNING) << "Reached end of the file";
                throw EOFError("Reached end of the file");
            }
            source.seekg(readChunks, std::ios::beg);
            auto file_data = bytes::make_unique_binary(size);
            source.read(reinterpret_cast<char*>(file_data.get()), size);
            readChunks += size;
            if (source.fail()) {
                RTC_LOG(LS_ERROR) << "Error while reading the file";
                throw FileError("Error while reading the file");
            }
            return std::move(file_data);
        });
    }
}
