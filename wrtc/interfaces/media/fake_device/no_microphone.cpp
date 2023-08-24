//
// Created by Laky64 on 24/08/2023.
//

#include "no_microphone.hpp"

namespace webrtc {
    rtc::scoped_refptr<AudioDeviceModule> NoMicrophone::Create(TaskQueueFactory *task_queue_factory) {
        return TestAudioDeviceModule::Create(
                task_queue_factory,
                std::move(ZeroCapturer::Create(48000)),
                std::move(TestAudioDeviceModule::CreateDiscardRenderer(48000))
        );
    }
} // wrtc