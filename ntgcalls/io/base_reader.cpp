//
// Created by Laky64 on 04/08/2023.
//

#include "base_reader.hpp"
#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    BaseReader::BaseReader(const bool noLatency): noLatency(noLatency) {
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
        if (eof()) {
            return nullptr;
        }
        if (noLatency) {
            try {
                return readInternal(size);
            } catch (...) {
                _eof = true;
            }
            return nullptr;
        }
        if (!dispatchQueue) return nullptr;
        std::shared_lock lock(mutex);
        if (nextBuffer.size() <= 4 && !running) {
            running = true;
            promise = std::make_shared<std::promise<void>>();
            const auto availableSpace = 10 - nextBuffer.size();
            dispatchQueue->dispatch([this, size, availableSpace]{
                std::vector<wrtc::binary> tmpBuffer;
                try {
                    for (int i = 0; i < availableSpace; i++) {
                        if (auto tmp = readInternal(size); tmp) {
                            tmpBuffer.push_back(tmp);
                        }
                    }
                } catch (...) {
                    std::lock_guard writeLock(mutex);
                    _eof = true;
                }
                std::lock_guard writeLock(mutex);
                nextBuffer.insert(nextBuffer.end(), tmpBuffer.begin(), tmpBuffer.end());
                running = false;
                if (promise) promise->set_value();
            });
        }
        if (nextBuffer.empty() && !_eof) {
            lock.unlock();
            if (promise) promise->get_future().wait();
            lock.lock();
        }
        wrtc::binary res = nullptr;
        if (!nextBuffer.empty()) {
            res = nextBuffer[0];
            nextBuffer.erase(nextBuffer.begin());
        }
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
