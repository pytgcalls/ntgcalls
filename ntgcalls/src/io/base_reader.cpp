//
// Created by Laky64 on 28/09/24.
//

#include <ntgcalls/io/base_reader.hpp>

namespace ntgcalls {
    BaseReader::BaseReader(BaseSink *sink): sink(sink) {}

    BaseReader::~BaseReader() {
        sink = nullptr;
    }

    void BaseReader::onEof(const std::function<void()>& callback) {
        eofCallback = callback;
    }

    void BaseReader::onData(const std::function<void(bytes::unique_binary)>& callback) {
        dataCallback = callback;
    }

    bool BaseReader::set_enabled(const bool status) {
        return !std::exchange(enabled, status);
    }

    bool BaseReader::is_enabled() const {
        return enabled;
    }
} // ntgcalls