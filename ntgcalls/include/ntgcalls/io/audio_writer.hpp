//
// Created by Lauren on 07/10/24.
//

#pragma once

#include <map>
#include <ntgcalls/io/base_writer.hpp>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::io {

    class AudioWriter: public BaseWriter {
    public:
        explicit AudioWriter(media::BaseSink* sink): BaseWriter(sink) {}

        virtual void send_frames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) = 0;
    };

} // ntgcalls
