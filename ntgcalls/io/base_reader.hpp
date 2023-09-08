//
// Created by Laky64 on 04/08/2023.
//

#pragma once


#include <cstdint>
#include <vector>
#include <mutex>

#include <wrtc/wrtc.hpp>
#include "../utils/dispatch_queue.hpp"

namespace ntgcalls {
    class BaseReader {
    private:
        std::vector<wrtc::binary> nextBuffer;
        bool _eof = false;
        std::shared_ptr<DispatchQueue> dispatchQueue;

    protected:
        size_t readChunks = 0;

        BaseReader();

        virtual ~BaseReader();

        virtual wrtc::binary readInternal(size_t size) = 0;

    public:
        wrtc::binary read(size_t size);

        bool eof();

        virtual void close();
    };
}
