//
// Created by Laky64 on 23/06/2026.
//

#pragma once
#include <ntgcalls/io/audio_mixer.hpp>

#ifdef IS_MACOS
#include <queue>
#include <mutex>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/device_info.hpp>
#include <ntgcalls/devices/base_device_module.hpp>

namespace ntgcalls {

    class MacAudioDeviceModule final: public BaseDeviceModule, public BaseReader, public AudioMixer {
        static constexpr int kNumBuffers = 3;

        AudioQueueRef queue = nullptr;
        AudioQueueBufferRef buffers[kNumBuffers] = {};
        AudioStreamBasicDescription format = {};
        std::string deviceUID;
        uint32_t bufferBytes = 0;
        std::mutex queueMutex;
        std::queue<bytes::unique_binary> dataQueue;

        void onData(bytes::unique_binary data) override;

        void setQueueDevice() const;

        static std::string cfStringToStd(CFStringRef value);

        static std::string stringProperty(AudioDeviceID id, AudioObjectPropertySelector selector);

        static UInt32 channelsForScope(AudioDeviceID id, AudioObjectPropertyScope scope);

        static void inputCallback(void* userData, AudioQueueRef aq, AudioQueueBufferRef buffer, const AudioTimeStamp* startTime, UInt32 numPackets, const AudioStreamPacketDescription* packetDescs);

        static void outputCallback(void* userData, AudioQueueRef aq, AudioQueueBufferRef buffer);

        void handleInput(AudioQueueBufferRef buffer);

        void handleOutput(AudioQueueBufferRef buffer);

    public:
        MacAudioDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink *sink);

        ~MacAudioDeviceModule() override;

        static bool isSupported();

        static std::vector<DeviceInfo> getDevices();

        void open() override;
    };

} // ntgcalls

#endif