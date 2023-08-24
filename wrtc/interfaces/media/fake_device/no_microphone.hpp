//
// Created by Laky64 on 24/08/2023.
//

#pragma once

#include <rtc_base/buffer.h>
#include <api/make_ref_counted.h>
#include <modules/audio_device/include/audio_device.h>

#include "zero_capturer.hpp"

namespace webrtc {
    class NoMicrophone {
    public:
        static rtc::scoped_refptr<webrtc::AudioDeviceModule> Create(webrtc::TaskQueueFactory* task_queue_factory);
    };
} // wrtc
