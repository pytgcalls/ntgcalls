//
// Created by Lauren on 07/10/24.
//

#pragma once

#include <condition_variable>
#include <queue>
#include <ntgcalls/io/audio_mixer.hpp>
#include <rtc_base/platform_thread.h>

namespace ntgcalls::io {

    class ThreadedAudioMixer: public AudioMixer {
        std::mutex queue_mutex_;
        std::queue<bytes::unique_binary> queue_;
        std::mutex mtx_;
        std::condition_variable cv_;
        webrtc::PlatformThread thread_;

    protected:
        void on_data(bytes::unique_binary data) override;

        virtual void write(const bytes::unique_binary& data) = 0;

    public:
        explicit ThreadedAudioMixer(media::BaseSink* sink);

        ~ThreadedAudioMixer() override;

        void open() override;
    };

} // ntgcalls::io
