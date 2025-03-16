//
// Created by Laky64 on 07/10/24.
//

#pragma once
#include <ntgcalls/io/base_io.hpp>

namespace ntgcalls {

    class BaseWriter: public virtual BaseIO {
    public:
        explicit BaseWriter(BaseSink* sink): BaseIO(sink) {}

        virtual void open() = 0;
    };

} // ntgcalls
