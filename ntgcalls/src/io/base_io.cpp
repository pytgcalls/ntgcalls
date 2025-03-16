//
// Created by Laky64 on 08/10/24.
//

#include <ntgcalls/io/base_io.hpp>

namespace ntgcalls {

    BaseIO::BaseIO(BaseSink* sink): sink(sink) {}

    void BaseIO::onEof(const std::function<void()>& callback) {
        eofCallback = callback;
    }

    BaseIO::~BaseIO() {
        sink = nullptr;
    }

} // ntgcalls