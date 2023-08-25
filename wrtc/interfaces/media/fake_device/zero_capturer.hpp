//
// Created by Laky64 on 24/08/2023.
//
#pragma once

#include <memory>
#include <modules/audio_device/include/test_audio_device.h>

namespace webrtc {

    class ZeroCapturer: public TestAudioDeviceModule::Capturer {
    public:
        explicit ZeroCapturer(int sampling_frequency_in_hz): _sampling_frequency_in_hz(sampling_frequency_in_hz) {}

        int SamplingFrequency() const override {
            return _sampling_frequency_in_hz;
        }

        bool Capture(rtc::BufferT<int16_t>* buffer) override {
            if (!_produced_output) {
                buffer->SetSize(TestAudioDeviceModule::SamplesPerFrame(_sampling_frequency_in_hz));
                _produced_output = true;
            }
            return false;
        }

        int NumChannels() const override {
            return 2;
        }

        static std::unique_ptr<ZeroCapturer> Create(int sampling_frequency_in_hz) {
            return std::make_unique<ZeroCapturer>(sampling_frequency_in_hz);
        }

    private:
        int _sampling_frequency_in_hz;
        bool _produced_output = false;
    };

}  // namespace wrtc
