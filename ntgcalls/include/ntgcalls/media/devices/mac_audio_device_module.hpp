//
// Created by Lauren on 23/06/26.
//

#pragma once

#ifdef IS_MACOS
#include <mutex>
#include <queue>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <ntgcalls/io/audio_mixer.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/devices/base_device_module.hpp>
#include <ntgcalls/media/devices/device_info.hpp>

namespace ntgcalls::media::devices {

    class MacAudioDeviceModule final: public BaseDeviceModule, public io::BaseReader, public io::AudioMixer {
        static constexpr int kNumBuffers = 3;

        AudioQueueRef queue_ = nullptr;
        AudioQueueBufferRef buffers_[kNumBuffers] = {};
        AudioStreamBasicDescription format_ = {};
        std::string device_uid_;
        uint32_t buffer_bytes_ = 0;
        std::mutex queue_mutex_;
        std::queue<bytes::unique_binary> data_queue_;

        void set_queue_device() const;

        static std::string cf_string_to_std(CFStringRef value);

        static std::string string_property(AudioDeviceID id, AudioObjectPropertySelector selector);

        static UInt32 channels_for_scope(AudioDeviceID id, AudioObjectPropertyScope scope);

        static void input_callback(void* user_data, AudioQueueRef aq, AudioQueueBufferRef buffer, const AudioTimeStamp* start_time, UInt32 num_packets, const AudioStreamPacketDescription* packet_desc);

        static void output_callback(void* user_data, AudioQueueRef aq, AudioQueueBufferRef buffer);

        void handle_input(AudioQueueBufferRef buffer) const;

        void handle_output(AudioQueueBufferRef buffer);

    protected:
        void on_data(bytes::unique_binary data) override;

    public:
        MacAudioDeviceModule(const AudioDescription* desc, bool is_capture, BaseSink* sink);

        ~MacAudioDeviceModule() override;

        static bool is_supported();

        static std::vector<DeviceInfo> get_devices();

        void open() override;
    };

} // ntgcalls::media::devices

#endif
