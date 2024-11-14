//
// Created by Laky64 on 07/10/24.
//

#include <ntgcalls/io/threaded_audio_mixer.hpp>

namespace ntgcalls {
    ThreadedAudioMixer::ThreadedAudioMixer(BaseSink* sink): AudioMixer(sink) {}

    ThreadedAudioMixer::~ThreadedAudioMixer() {
        exiting = true;
        const bool wasRunning = running;
        if (running) {
            running = false;
            cv.notify_all();
        }
        if (wasRunning) thread.Finalize();
    }

    void ThreadedAudioMixer::open() {
        if (running) return;
        running = true;
        auto frameSize = sink->frameSize();
        auto frameTime = sink->frameTime();
        thread = rtc::PlatformThread::SpawnJoinable(
        [this, frameSize, frameTime] {
                while (running) {
                    std::unique_lock lock(mtx);
                    const auto ok = cv.wait_for(lock, frameTime + std::chrono::milliseconds(20), [this] {
                        std::lock_guard queueLock(queueMutex);
                        return !queue.empty() || !running;
                    });
                    if (!running) {
                        break;
                    }
                    try {
                        if (ok) {
                            std::lock_guard queueLock(queueMutex);
                            write(queue.front());
                            queue.pop();
                        } else {
                            write(bytes::make_unique_binary(frameSize));
                        }
                    } catch (...) {
                        running = false;
                        break;
                    }
                }
                if (!exiting) (void) eofCallback();
            },
            "ThreadedMixer",
            rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime)
        );
    }

    void ThreadedAudioMixer::onData(bytes::unique_binary data) {
        std::lock_guard queueLock(queueMutex);
        queue.push(std::move(data));
        cv.notify_one();
    }
} // ntgcalls