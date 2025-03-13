//
// Created by Laky64 on 28/09/24.
//

#include <thread>
#include <ntgcalls/io/threaded_reader.hpp>

namespace ntgcalls {
    ThreadedReader::ThreadedReader(BaseSink *sink, const size_t threadCount): BaseReader(sink) {
        bufferThreads.reserve(threadCount);
    }

    void ThreadedReader::close() {
        dataCallback = nullptr;
        exiting = true;
        const bool wasRunning = running;
        if (running) {
            running = false;
            cv.notify_all();
        }
        if (wasRunning) {
            for (auto& thread : bufferThreads) {
                thread.Finalize();
            }
        }
    }

    void ThreadedReader::run(const std::function<bytes::unique_binary(int64_t)>& readCallback) {
        if (running) return;
        const size_t bufferCount = bufferThreads.capacity();
        running = true;
        auto frameTime = sink->frameTime();
        for (size_t i = 0; i < bufferCount; ++i) {
            bufferThreads.push_back(
                rtc::PlatformThread::SpawnJoinable(
                    [this, i, bufferCount, frameSize = sink->frameSize(), maxBufferSize = std::chrono::seconds(1) / frameTime / 10, frameTime, readCallback] {
                        activeBufferCount++;
                        std::vector<bytes::unique_binary> frames;
                        frames.reserve(maxBufferSize);
                        while (running) {
                            std::unique_lock lock(mtx);
                            try {
                                auto data = std::move(readCallback(frameSize * maxBufferSize));
                                frames.clear();
                                for (size_t j = 0; j < maxBufferSize; j++) {
                                    const size_t offset = j * frameSize;
                                    auto chunk = bytes::make_unique_binary(frameSize);
                                    std::memcpy(chunk.get(), data.get() + offset, frameSize);
                                    frames.push_back(std::move(chunk));
                                }
                            } catch (...) {
                                running = false;
                                break;
                            }
                            cv.wait(lock, [this, i] {
                                return !running || (activeBuffer == i && enabled);
                            });
                            if (!running) break;
                            for (auto& chunk : frames) {
                                if (!running) break;
                                dataCallback(std::move(chunk), {});
                                std::this_thread::sleep_for(frameTime);
                            }
                            activeBuffer = (activeBuffer + 1) % bufferCount;
                            lock.unlock();
                            cv.notify_all();
                        }
                        std::lock_guard lock(mtx);
                        activeBufferCount--;
                        if (activeBufferCount == 0) {
                            if (!exiting) (void) eofCallback();
                        } else {
                            cv.notify_all();
                        }
                    },
                    "ThreadedReader_" + std::to_string(bufferCount),
                    rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime)
                )
            );
        }
    }

    bool ThreadedReader::set_enabled(const bool status) {
        const auto res = BaseReader::set_enabled(status);
        cv.notify_all();
        return res;
    }
} // ntgcalls