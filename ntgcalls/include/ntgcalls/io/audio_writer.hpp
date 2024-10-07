//
// Created by Laky64 on 07/10/24.
//

#pragma once

#include <map>
#include <ntgcalls/io/base_writer.hpp>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls {

    class AudioWriter: public BaseWriter {
    public:
        virtual void sendFrames(const std::map<uint32_t, bytes::shared_binary>& frames) = 0;
    };

} // ntgcalls
