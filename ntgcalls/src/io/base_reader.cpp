//
// Created by Laky64 on 28/09/24.
//

#include <utility>
#include <ntgcalls/io/base_reader.hpp>

namespace ntgcalls {
    BaseReader::BaseReader(BaseSink *sink): BaseIO(sink) {}

    void BaseReader::onData(const std::function<void(bytes::unique_binary, wrtc::FrameData)>& callback) {
        dataCallback = callback;
    }

    bool BaseReader::set_enabled(const bool status) {
        return !std::exchange(enabled, status);
    }

    bool BaseReader::is_enabled() const {
        return enabled;
    }
} // ntgcalls