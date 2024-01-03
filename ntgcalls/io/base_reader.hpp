//
// Created by Laky64 on 04/08/2023.
//

#pragma once


#include <vector>

#include <wrtc/wrtc.hpp>
#include "../utils/dispatch_queue.hpp"

namespace ntgcalls {
    class BaseReader {
        std::vector<wrtc::binary> nextBuffer;
        bool _eof = false, running = false;
        std::shared_ptr<DispatchQueue> dispatchQueue;
        std::recursive_mutex mutex;
        std::shared_ptr<std::promise<void>> promise;

    protected:
        int64_t readChunks = 0;

        BaseReader();

        virtual ~BaseReader();

        virtual wrtc::binary readInternal(int64_t size) = 0;

    public:
        wrtc::binary read(int64_t size);

        [[nodiscard]] bool eof() const;

        virtual void close();
    };
}
