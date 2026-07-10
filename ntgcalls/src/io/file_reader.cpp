//
// Created by Lauren on 04/08/23.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/file_reader.hpp>

namespace ntgcalls::io {
    FileReader::FileReader(const std::string& path, media::BaseSink *sink): BaseIO(sink), ThreadedReader(sink) {
        source_ = std::ifstream(path, std::ios::binary);
        if (!source_) {
            RTC_LOG(LS_ERROR) << "Unable to open the file located at \"" << path << "\"";
            throw FileError("Unable to open the file located at \"" + path + "\"");
        }
    }

    FileReader::~FileReader() {
        close();
        if (source_.is_open()) {
            source_.close();
        }
        source_.clear();
        RTC_LOG(LS_VERBOSE) << "FileReader closed";
    }

    void FileReader::open() {
        run([this](const int64_t size) {
            if (!source_ || source_.eof() || source_.fail() || !source_.is_open()) {
                RTC_LOG(LS_WARNING) << "Reached end of the file";
                throw EOFError("Reached end of the file");
            }
            source_.seekg(read_chunks_, std::ios::beg);
            auto file_data = bytes::make_unique_binary(size);
            source_.read(reinterpret_cast<char*>(file_data.get()), size);
            read_chunks_ += size;
            if (source_.fail()) {
                RTC_LOG(LS_ERROR) << "Error while reading the file";
                throw FileError("Error while reading the file");
            }
            return std::move(file_data);
        });
    }
} // ntgcalls::io
