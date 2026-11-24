//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <vector>
#include <ntgcalls/io/base_io.hpp>
#include <wrtc/models/frame.hpp>

namespace ntgcalls::io {

    class BaseWriter: public virtual BaseIO {
    public:
        explicit BaseWriter(media::BaseSink* sink): BaseIO(sink) {}

        virtual void open() = 0;

        virtual void send_frames(std::vector<wrtc::models::Frame> frames) = 0;

        virtual void remove_source(int64_t) {}
    };

} // ntgcalls::io
