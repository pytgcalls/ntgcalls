//
// Created by Laky64 on 28/09/24.
//

#include <ntgcalls/io/base_reader.hpp>

namespace ntgcalls {
    BaseReader::BaseReader(BaseSink *sink): sink(sink) {}

    void BaseReader::onEof(const std::function<void()>& callback) {
        eofCallback = callback;
    }

    void BaseReader::onData(const std::function<void(bytes::unique_binary)>& callback) {
        dataCallback = callback;
    }
} // ntgcalls