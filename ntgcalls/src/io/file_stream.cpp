//
// Created by Lauren on 04/08/23.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/file_stream.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::io {
    FileStream::FileStream(const std::string& path, const Mode mode): file_(path, std::ios::binary | (mode == Mode::Read ? std::ios::in : std::ios::out | std::ios::trunc)) {
        if (!file_) {
            RTC_LOG(LS_ERROR) << "Unable to open the file located at \"" << path << "\"";
            throw FileError("Unable to open the file located at \"" + path + "\"");
        }
    }

    FileStream::~FileStream() {
        FileStream::close();
    }

    bytes::unique_binary FileStream::read(const int64_t size) {
        const std::lock_guard lock(mutex_);
        if (!file_ || file_.eof() || file_.fail() || !file_.is_open()) {
            RTC_LOG(LS_WARNING) << "Reached end of the file";
            throw EOFError("Reached end of the file");
        }
        file_.seekg(read_bytes_, std::ios::beg);
        auto data = bytes::make_unique_binary(size);
        file_.read(reinterpret_cast<char*>(data.get()), size);
        read_bytes_ += size;
        if (file_.eof()) {
            RTC_LOG(LS_WARNING) << "Reached end of the file";
            throw EOFError("Reached end of the file");
        }
        if (file_.fail()) {
            RTC_LOG(LS_ERROR) << "Error while reading the file";
            throw FileError("Error while reading the file");
        }
        return std::move(data);
    }

    void FileStream::write(const bytes::unique_binary& data, const size_t size) {
        const std::lock_guard lock(mutex_);
        if (!file_ || file_.fail() || !file_.is_open()) {
            RTC_LOG(LS_WARNING) << "Error while writing to the file";
            throw FileError("Error while writing to the file");
        }
        file_.write(reinterpret_cast<const char*>(data.get()), static_cast<std::streamsize>(size));
        if (file_.fail()) {
            RTC_LOG(LS_ERROR) << "Error while writing to the file";
            throw FileError("Error while writing to the file");
        }
    }

    void FileStream::close() {
        const std::lock_guard lock(mutex_);
        if (file_.is_open()) {
            file_.close();
            RTC_LOG(LS_VERBOSE) << "FileStream closed";
        }
        file_.clear();
    }
} // ntgcalls::io
