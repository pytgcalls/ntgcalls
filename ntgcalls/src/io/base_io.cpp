//
// Created by Lauren on 08/10/24.
//

#include <ntgcalls/io/base_io.hpp>

namespace ntgcalls::io {

    BaseIO::BaseIO(media::BaseSink* sink): sink_(sink) {}

    void BaseIO::on_eof(const std::function<void()>& callback) {
        eof_callback_ = callback;
    }

    BaseIO::~BaseIO() {
        eof_callback_ = nullptr;
        sink_ = nullptr;
    }

} // ntgcalls::io