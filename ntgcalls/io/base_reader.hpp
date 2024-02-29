//
// Created by Laky64 on 04/08/2023.
//

#pragma once


#include <shared_mutex>

#include <wrtc/wrtc.hpp>
#include "../utils/dispatch_queue.hpp"

namespace ntgcalls {
    class BaseReader {
        std::queue<wrtc::binary> buffer;
        std::mutex mutex;
        std::condition_variable bufferCondition;
        std::atomic_bool _eof = false, noLatency = false, quit = false;
        std::thread thread;
        int64_t size = 0;

    protected:
        int64_t readChunks = 0;

        explicit BaseReader(int64_t bufferSize, bool noLatency);

        virtual ~BaseReader();

        virtual wrtc::binary readInternal(int64_t size) = 0;

    public:
        std::pair<wrtc::binary, int64_t> read();

        [[nodiscard]] bool eof();

        virtual void close();

        void start();
    };
}
