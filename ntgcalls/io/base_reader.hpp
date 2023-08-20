//
// Created by Laky64 on 04/08/2023.
//

#pragma once


#include <cstdint>
#include <vector>

#include <wrtc/wrtc.hpp>

namespace ntgcalls {
    class BaseReader {
    protected:
        int64_t readChunks = 0;

    public:
        virtual wrtc::binary read(std::int64_t size) = 0;

        virtual ~BaseReader();

        virtual void close();

        virtual bool eof() = 0;
    };
}
