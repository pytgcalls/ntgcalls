//
// Created by Laky64 on 30/04/25.
//

#pragma once
#include <map>
#include <queue>
#include <memory>
#include <condition_variable>
#include <rtc_base/platform_thread.h>
#include <wrtc/models/audio_frame.hpp>

namespace wrtc {

    using namespace std::chrono_literals;

    class AudioThreadBuffer {
    public:
        struct Buffer {
            uint32_t ssrc;
            std::vector<int16_t> data;
            int sampleRate = 0;
            int channels = 0;
        };

        AudioThreadBuffer(
            const std::function<void(int)>& updateAudioSourceCountCallback,
            const std::function<void(std::unique_ptr<AudioFrame>)>& audioFrameCallback
        );

        ~AudioThreadBuffer();

        void sendData( std::map<uint32_t, std::unique_ptr<Buffer>>& interleavedAudioBySSRC);

    private:
        bool running = true;
        rtc::PlatformThread thread;
        std::condition_variable cv;
        std::mutex mtx, bufferMutex;
        std::chrono::steady_clock::time_point nextFrameTime;
        std::queue<std::vector<std::unique_ptr<Buffer>>> bufferQueue;
    };

} // wrtc
