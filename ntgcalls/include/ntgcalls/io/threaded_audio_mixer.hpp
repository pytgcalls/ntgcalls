//
// Created by Laky64 on 07/10/24.
//

#pragma once

#include <condition_variable>
#include <queue>
#include <ntgcalls/io/audio_mixer.hpp>
#include <rtc_base/platform_thread.h>

namespace ntgcalls {

    class ThreadedAudioMixer: public AudioMixer {
        std::mutex queueMutex;
        std::queue<bytes::unique_binary> queue;
        std::mutex mtx;
        std::condition_variable cv;
        rtc::PlatformThread thread;

        void onData(bytes::unique_binary data) override;

    protected:
        virtual void write(const bytes::unique_binary& data) = 0;

    public:
        explicit ThreadedAudioMixer(BaseSink* sink);

        ~ThreadedAudioMixer() override;

        void open() override;
    };

} // ntgcalls
