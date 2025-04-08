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
        auto frameTime = sink->frameTime();
        for (size_t i = 0; i < bufferCount; ++i) {
            bufferThreads.push_back(
                rtc::PlatformThread::SpawnJoinable(
                    [this, i, bufferCount, frequencyFrames = getFrameFrequency(), frameSize = sink->frameSize(), maxBufferSize = std::chrono::seconds(1) / frameTime / 10, frameTime, readCallback] {
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
                                    return !running || (activeBuffer == i && enabled);
                                });
                            }
                            if (!running) break;
                            for (auto& chunk : frames) {
                                if (!running) break;
                                dataCallback(std::move(chunk), {});
                                auto additionalTime = std::chrono::nanoseconds(0);
                                if (frequencyFrames > 0 && std::fmod(frameSent, frequencyFrames) < 1) {
                                    additionalTime = std::chrono::milliseconds(1);
                                }
                                std::this_thread::sleep_for(frameTime + additionalTime);
                                frameSent++;
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

    double_t ThreadedReader::getFrameFrequency() const {
        const auto missingMS  = std::chrono::milliseconds(1000) - std::chrono::duration_cast<std::chrono::milliseconds>(sink->frameTime()) * sink -> frameRate();
        double_t frequencyFrames = 0;
        if (missingMS.count() > 0) {
            frequencyFrames = sink -> frameRate() / static_cast<double_t>(missingMS.count());
        }
        return frequencyFrames;
    }
} // ntgcalls