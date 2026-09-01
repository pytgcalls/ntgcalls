//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <ntgcalls/io/base_io.hpp>

namespace ntgcalls::io {

    class BaseWriter: public virtual BaseIO {
    public:
        explicit BaseWriter(media::BaseSink* sink): BaseIO(sink) {}

        virtual void open() = 0;
    };

} // ntgcalls
