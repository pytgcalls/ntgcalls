//
// Created by Laky64 on 04/08/2023.
//

#pragma once


#include <shared_mutex>
#include <vector>

#include <wrtc/wrtc.hpp>
#include "../utils/dispatch_queue.hpp"

namespace ntgcalls {
    class BaseReader {
        std::vector<wrtc::binary> buffer;
        wrtc::binary currentBuffer;
        std::condition_variable bufferCondition;
        std::atomic_bool _eof = false, running = false, noLatency = false, quit = false;
        std::thread thread;
        std::mutex mutex;
        std::shared_ptr<std::promise<void>> promise;
        int64_t size = 0;

        void readAsync();

    protected:
        int64_t readChunks = 0;

        explicit BaseReader(int64_t bufferSize, bool noLatency);

        virtual ~BaseReader();

        virtual wrtc::binary readInternal(int64_t size) = 0;

    public:
        wrtc::binary read();

        [[nodiscard]] bool eof();

        virtual void close();

        void start();
    };
}
