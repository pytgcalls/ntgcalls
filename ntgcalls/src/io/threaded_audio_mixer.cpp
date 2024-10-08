//
// Created by Laky64 on 07/10/24.
//

#include <ntgcalls/io/threaded_audio_mixer.hpp>

namespace ntgcalls {
    ThreadedAudioMixer::ThreadedAudioMixer(BaseSink* sink): AudioMixer(sink) {}

    ThreadedAudioMixer::~ThreadedAudioMixer() {
        running = false;
        cv.notify_one();
        thread.Finalize();
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
                        return !queue.empty() || !running;
                    });
                    if (!running) {
                        return;
                    }
                    try {
                        if (ok) {
                            write(queue.front());
                            queue.pop();
                        } else {
                            write(bytes::make_unique_binary(frameSize));
                        }
                    } catch (...) {
                        break;
                    }
                }
                (void) eofCallback();
            },
            "ThreadedMixer",
            rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime)
        );
    }

    void ThreadedAudioMixer::onData(bytes::unique_binary data) {
        queue.push(std::move(data));
        cv.notify_one();
    }
} // ntgcalls