//
// Created by Laky64 on 28/09/24.
//

#include <thread>
#include <ntgcalls/io/threaded_reader.hpp>

namespace ntgcalls {
    ThreadedReader::ThreadedReader(BaseSink *sink, const size_t threadCount): BaseReader(sink), SyncHelper(sink->frameTime()) {
        bufferThreads.reserve(threadCount);
    }

    void ThreadedReader::close() {
        {
            std::lock_guard lock(mtx);
            dataCallback = nullptr;
            eofCallback = nullptr;
            running = false;
            cv.notify_all();
        }
        for (auto& thread : bufferThreads) {
            thread.Finalize();
        }
    }

    void ThreadedReader::run(const std::function<bytes::unique_binary(int64_t)>& readCallback) {
        if (running) return;
        const auto bufferCount = bufferThreads.capacity();
        running = true;
        synchronizeTime();
        const auto frameTime = sink->frameTime();
        for (size_t i = 0; i < bufferCount; ++i) {
            bufferThreads.push_back(
                webrtc::PlatformThread::SpawnJoinable(
                    [this, i, bufferCount, frameSize = sink->frameSize(), maxBufferSize = std::chrono::seconds(1) / frameTime / 10, readCallback] {
                        activeBufferCount++;
                        std::vector<bytes::unique_binary> frames;
                        frames.reserve(maxBufferSize);
                        while (running) {
                            try {
                                std::unique_lock lock(mtx);
                                auto data = readCallback(frameSize * maxBufferSize);
                                lock.unlock();
                                frames.clear();
                                for (size_t j = 0; j < maxBufferSize; j++) {
                                    const size_t offset = j * frameSize;
                                    auto chunk = bytes::make_unique_binary(frameSize);
                                    std::memcpy(chunk.get(), data.get() + offset, frameSize);
                                    frames.push_back(std::move(chunk));
                                }
                            } catch (...) {
                                std::lock_guard lock(mtx);
                                running = false;
                                cv.notify_all();
                                break;
                            }
                            {
                                std::unique_lock lock(mtx);
                                cv.wait(lock, [this, i] {
                                    return !running || (activeBuffer == i && status);
                                });
                            }
                            if (!running) break;
                            for (auto& chunk : frames) {
                                if (!running) break;
                                dataCallback(std::move(chunk), {});
                                waitNextFrame();
                            }
                            activeBuffer = (activeBuffer + 1) % bufferCount;
                            cv.notify_all();
                        }
                        std::lock_guard lock(mtx);
                        activeBufferCount--;
                        if (activeBufferCount == 0) {
                            (void) eofCallback();
                        }
                    },
                    "ThreadedReader_" + std::to_string(bufferCount),
                    webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime)
                )
            );
        }
    }

    bool ThreadedReader::set_enabled(const bool enable) {
        const auto res = BaseReader::set_enabled(enable);
        cv.notify_all();
        return res;
    }
} // ntgcalls