//
// Created by Lauren on 28/09/24.
//

#include <utility>
#include <ntgcalls/io/base_reader.hpp>

namespace ntgcalls::io {
    BaseReader::BaseReader(media::BaseSink* sink): BaseIO(sink) {}

    BaseReader::~BaseReader() {
        data_callback_ = nullptr;
    }

    void BaseReader::on_data(const std::function<void(bytes::unique_binary, wrtc::models::FrameData)>& callback) {
        data_callback_ = callback;
    }

    bool BaseReader::set_enabled(const bool enable) {
        return std::exchange(status_, enable) != enable;
    }

    bool BaseReader::is_enabled() const {
        return status_;
    }
} // ntgcalls::io
