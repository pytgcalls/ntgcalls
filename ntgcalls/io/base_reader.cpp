//
// Created by Laky64 on 04/08/2023.
//

#include "base_reader.hpp"
#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    BaseReader::BaseReader(const int64_t bufferSize, const bool noLatency): noLatency(noLatency) {
        size = bufferSize;
    }

    BaseReader::~BaseReader() {
        BaseReader::close();
        std::lock_guard lock(mutex);
        promise = nullptr;
        readChunks = 0;
        buffer.clear();
    }

    void BaseReader::start() {
        if (!noLatency) thread = std::thread(&BaseReader::readAsync, this);
    }

    void BaseReader::readAsync() {
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            std::unique_lock lock(mutex);
            if (buffer.size() < 10 && !_eof) {
                const auto availableSpace = 10 - buffer.size();
                try {
                    for (int i = 0; i < availableSpace; i++) {
                        if (auto tmp = this->readInternal(size); tmp) {
                            buffer.push_back(tmp);
                        }
                    }
                } catch (...) {
                    _eof = true;
                }
            }
            if (!currentBuffer) {
                currentBuffer = buffer[0];
                buffer.erase(buffer.begin());
                lock.unlock();
                bufferCondition.notify_one();
            }
        } while (!quit);
    }

    wrtc::binary BaseReader::read() {
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
        std::unique_lock lock(mutex);
        wrtc::binary res = nullptr;
        bufferCondition.wait(lock, [this]{
            return currentBuffer || quit || _eof;
        });
        res = currentBuffer;
        currentBuffer = nullptr;
        return res;
    }

    void BaseReader::close() {
        quit = true;
        if(thread.joinable()) {
            thread.join();
        }
    }

    bool BaseReader::eof() {
        std::lock_guard lock(mutex);
        return _eof && buffer.empty();
    }
}
