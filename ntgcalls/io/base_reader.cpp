//
// Created by Laky64 on 04/08/2023.
//

#include "base_reader.hpp"
#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    BaseReader::BaseReader(const int64_t bufferSize, const bool noLatency): noLatency(noLatency), size(bufferSize) {}

    BaseReader::~BaseReader() {
        BaseReader::close();
        readChunks = 0;
    }

    void BaseReader::start() {
        if (!noLatency) {
            thread = std::thread([this] {
                do {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    std::unique_lock lock(mutex);
                    const auto availableSpace = 10 - buffer.size();
                    lock.unlock();
                    for (int i = 0; i < availableSpace; i++) {
                        try {
                            if (auto tmp = this->readInternal(size); tmp) {
                                lock.lock();
                                buffer.push(std::move(tmp));
                                lock.unlock();
                                bufferCondition.notify_one();
                            }
                        } catch (...) {
                            lock.lock();
                            _eof = true;
                            lock.unlock();
                            bufferCondition.notify_one();
                            break;
                        }
                    }
                } while (!quit && !_eof);
            });
        }
    }

    std::pair<bytes::shared_binary, int64_t> BaseReader::read() {
        auto timeMillis = rtc::TimeMillis();
        if (eof()) {
            return {nullptr, timeMillis};
        }
        if (noLatency) {
            try {
                return {readInternal(size), timeMillis};
            } catch (...) {
                _eof = true;
            }
            return {nullptr, timeMillis};
        }
        std::unique_lock lock(mutex);
        bufferCondition.wait_for(lock, std::chrono::milliseconds(500), [this] {
            return !buffer.empty() || quit || _eof;
        });
        if (buffer.empty()) {
            return {nullptr, timeMillis};
        }
        auto data = std::move(buffer.front());
        buffer.pop();
        return {data, timeMillis};
    }

    void BaseReader::close() {
        RTC_LOG(LS_VERBOSE) << "Closing reader";
        quit = true;
        if(thread.joinable()) {
            thread.join();
        }
        RTC_LOG(LS_VERBOSE) << "Reader closed";
    }

    bool BaseReader::eof() {
        std::lock_guard lock(mutex);
        return _eof && buffer.empty();
    }
}
