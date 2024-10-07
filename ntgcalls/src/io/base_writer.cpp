//
// Created by Laky64 on 07/10/24.
//

#include <ntgcalls/io/base_writer.hpp>

namespace ntgcalls {
    void BaseWriter::onEof(const std::function<void()>& callback) {
        eofCallback = callback;
    }
} // ntgcalls