//
// Created by Laky64 on 30/04/25.
//

#include <thread>
#include <ranges>
#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/audio_thread_buffer.hpp>

namespace wrtc {
    AudioThreadBuffer::AudioThreadBuffer(
        const std::function<void(int)>& updateAudioSourceCountCallback,
        const std::function<void(std::unique_ptr<AudioFrame>)>& audioFrameCallback
    ) {
        thread = rtc::PlatformThread::SpawnJoinable(
            [this, updateAudioSourceCountCallback, audioFrameCallback] {
                while (running) {
                    std::unique_lock lock(mtx);
                    cv.wait(lock, [this] {
                        std::lock_guard queueLock(bufferMutex);
                        return !bufferQueue.empty() || !running;
                    });

                    if (!running) {
                        break;
                    }

                    std::unique_lock queueLock(bufferMutex);
                    auto &frames = bufferQueue.front();
                    updateAudioSourceCountCallback(static_cast<int>(frames.size()));
                    for (const auto &buffer : frames) {
                        auto frame = std::make_unique<AudioFrame>(buffer->ssrc);
                        frame->channels = buffer->channels;
                        frame->sampleRate = buffer->sampleRate;
                        frame->data = buffer->data.data();
                        frame->size = buffer->data.size() * sizeof(int16_t);
                        audioFrameCallback(std::move(frame));
                    }
                    bufferQueue.pop();
                    queueLock.unlock();
                    nextFrameTime += 10ms;
                    std::this_thread::sleep_until(nextFrameTime);
                }
            },
            "AudioStreamThread",
            rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime));
    }
    AudioThreadBuffer::~AudioThreadBuffer() {
        running = false;
        cv.notify_all();
        thread.Finalize();
    }

    void AudioThreadBuffer::sendData(std::map<uint32_t, std::unique_ptr<Buffer>>& interleavedAudioBySSRC) {
        std::lock_guard lock(bufferMutex);
        if (nextFrameTime == std::chrono::steady_clock::time_point()) {
            nextFrameTime = std::chrono::steady_clock::now();
        }

        std::vector<std::unique_ptr<Buffer>> frames;
        frames.reserve(interleavedAudioBySSRC.size());

        for (auto& buffer : interleavedAudioBySSRC | std::views::values) {
            frames.push_back(std::move(buffer));
        }

        bufferQueue.push(std::move(frames));
        cv.notify_one();
    }
} // wrtc