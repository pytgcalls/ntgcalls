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
        running = true;
        thread = rtc::PlatformThread::SpawnJoinable(
        [this] {
                while (running) {
                    std::unique_lock lock(mtx);
                    const auto ok = cv.wait_for(lock, sink->frameTime() + std::chrono::milliseconds(20), [this] {
                        return !queue.empty() || !running;
                    });
                    try {
                        if (ok) {
                            write(queue.front());
                            queue.pop();
                        } else {
                            write(bytes::make_unique_binary(sink->frameSize()));
                        }
                    } catch (...) {
                        break;
                    }
                }
                (void) eofCallback();
            },
            "ThreadedAudioMixer",
            rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime)
        );
    }

    void ThreadedAudioMixer::onData(bytes::unique_binary data) {
        queue.push(std::move(data));
        cv.notify_one();
    }
} // ntgcalls