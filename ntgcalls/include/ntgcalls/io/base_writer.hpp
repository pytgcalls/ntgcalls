//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <map>
#include <ntgcalls/io/base_io.hpp>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::io {

    class BaseWriter: public virtual BaseIO {
    public:
        explicit BaseWriter(media::BaseSink* sink): BaseIO(sink) {}

        virtual void open() = 0;

        virtual void send_frames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) = 0;
    };

} // ntgcalls::io
