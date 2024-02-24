//
// Created by Laky64 on 04/08/2023.
//

#include "base_reader.hpp"
#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    BaseReader::BaseReader() {
        dispatchQueue = std::make_shared<DispatchQueue>();
    }

    BaseReader::~BaseReader() {
        BaseReader::close();
        std::lock_guard lock(mutex);
        promise = nullptr;
        readChunks = 0;
        nextBuffer.clear();
    }

    wrtc::binary BaseReader::read(int64_t size) {
        wrtc::binary res = nullptr;
        if (!dispatchQueue) return res;
        mutex.lock();
        promise = std::make_shared<std::promise<void>>();
        if (!_eof && nextBuffer.size() <= 4 && !running) {
            running = true;
            const auto availableSpace = 10 - nextBuffer.size();
            dispatchQueue->dispatch([this, size, availableSpace] {
                try {
                    for (int i = 0; i < availableSpace; i++) {
                        if (auto tmp = readInternal(size); tmp) {
                            mutex.lock();
                            nextBuffer.push_back(tmp);
                            mutex.unlock();
                        }
                    }
                } catch (...) {
                    mutex.lock();
                    _eof = true;
                    mutex.unlock();
                }
                mutex.lock();
                running = false;
                if (promise) promise->set_value();
                mutex.unlock();
            });
        }
        if (nextBuffer.empty() && !_eof) {
            mutex.unlock();
            if (promise) promise->get_future().wait();
            mutex.lock();
        }
        if (!nextBuffer.empty()) {
            res = nextBuffer[0];
            nextBuffer.erase(nextBuffer.begin());
        }
        mutex.unlock();
        return res;
    }

    void BaseReader::close() {
        dispatchQueue = nullptr;
    }

    bool BaseReader::eof() {
        std::lock_guard lock(mutex);
        return _eof && nextBuffer.empty();
    }
}
