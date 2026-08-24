//
// Created by Lauren on 23/06/26.
//

#ifdef IS_MACOS
#include <vector>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/devices/mac_audio_device_module.hpp>

namespace ntgcalls::media::devices {
    MacAudioDeviceModule::MacAudioDeviceModule(const AudioDescription* desc, const bool is_capture, BaseSink* sink):
    BaseIO(sink), BaseDeviceModule(desc, is_capture), BaseReader(sink), AudioMixer(sink) {
        try {
            device_uid_ = device_metadata_["uid"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        format_.mSampleRate = rate_;
        format_.mFormatID = kAudioFormatLinearPCM;
        format_.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
        format_.mBitsPerChannel = 16;
        format_.mChannelsPerFrame = channels_;
        format_.mFramesPerPacket = 1;
        format_.mBytesPerFrame = format_.mChannelsPerFrame * (format_.mBitsPerChannel / 8);
        format_.mBytesPerPacket = format_.mBytesPerFrame * format_.mFramesPerPacket;
    }

    MacAudioDeviceModule::~MacAudioDeviceModule() {
        running_ = false;
        if (queue_) {
            AudioQueueStop(queue_, true);
            AudioQueueDispose(queue_, true);
            queue_ = nullptr;
        }
    }

    bool MacAudioDeviceModule::is_supported() {
        return true;
    }

    std::vector<DeviceInfo> MacAudioDeviceModule::get_devices() {
        std::vector<DeviceInfo> devices;
        constexpr AudioObjectPropertyAddress kDevicesAddr = {
            kAudioHardwarePropertyDevices,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        UInt32 data_size = 0;
        if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &kDevicesAddr, 0, nullptr, &data_size) != noErr || data_size == 0) {
            return devices;
        }
        std::vector<AudioDeviceID> ids(data_size / sizeof(AudioDeviceID));
        if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &kDevicesAddr, 0, nullptr, &data_size, ids.data()) != noErr) {
            return devices;
        }

        for (const auto id : ids) {
            const auto uid = string_property(id, kAudioDevicePropertyDeviceUID);
            if (uid.empty()) {
                continue;
            }
            const auto name = string_property(id, kAudioObjectPropertyName);
            if (channels_for_scope(id, kAudioObjectPropertyScopeInput) > 0) {
                const json metadata = {
                    {"is_microphone", true},
                    {"uid", uid},
                };
                devices.emplace_back(name, metadata.dump());
            }
            if (channels_for_scope(id, kAudioObjectPropertyScopeOutput) > 0) {
                const json metadata = {
                    {"is_microphone", false},
                    {"uid", uid},
                };
                devices.emplace_back(name, metadata.dump());
            }
        }
        return devices;
    }

    std::string MacAudioDeviceModule::cf_string_to_std(CFStringRef value) {
        if (!value) {
            return {};
        }
        const CFIndex length = CFStringGetLength(value);
        const CFIndex max_bytes = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        std::string out(max_bytes, '\0');
        if (CFStringGetCString(value, out.data(), max_bytes, kCFStringEncodingUTF8)) {
            out.resize(std::strlen(out.c_str()));
            return out;
        }
        return {};
    }

    std::string MacAudioDeviceModule::string_property(const AudioDeviceID id, const AudioObjectPropertySelector selector) {
        const AudioObjectPropertyAddress addr = {selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
        CFStringRef value = nullptr;
        UInt32 size = sizeof(CFStringRef);
        if (AudioObjectGetPropertyData(id, &addr, 0, nullptr, &size, &value) != noErr || !value) {
            return {};
        }
        auto result = cf_string_to_std(value);
        CFRelease(value);
        return result;
    }

    UInt32 MacAudioDeviceModule::channels_for_scope(const AudioDeviceID id, const AudioObjectPropertyScope scope) {
        const AudioObjectPropertyAddress addr = {kAudioDevicePropertyStreamConfiguration, scope, kAudioObjectPropertyElementMain};
        UInt32 size = 0;
        if (AudioObjectGetPropertyDataSize(id, &addr, 0, nullptr, &size) != noErr || size == 0) {
            return 0;
        }
        std::vector<uint8_t> storage(size);
        const auto buffer_list = reinterpret_cast<AudioBufferList*>(storage.data());
        if (AudioObjectGetPropertyData(id, &addr, 0, nullptr, &size, buffer_list) != noErr) {
            return 0;
        }
        UInt32 count = 0;
        for (UInt32 i = 0; i < buffer_list->mNumberBuffers; i++) {
            count += buffer_list->mBuffers[i].mNumberChannels;
        }
        return count;
    }

    void MacAudioDeviceModule::set_queue_device() const {
        if (device_uid_.empty()) {
            return;
        }
        const CFStringRef uid_ref = CFStringCreateWithCString(nullptr, device_uid_.c_str(), kCFStringEncodingUTF8);
        if (!uid_ref) {
            return;
        }
        AudioQueueSetProperty(queue_, kAudioQueueProperty_CurrentDevice, &uid_ref, sizeof(CFStringRef));
        CFRelease(uid_ref);
    }

    void MacAudioDeviceModule::open() {
        if (running_) return;
        buffer_bytes_ = static_cast<uint32_t>(sink_->frame_size());
        if (buffer_bytes_ == 0) {
            throw MediaDeviceError("Invalid audio frame size");
        }
        running_ = true;
        if (is_capture_) {
            if (AudioQueueNewInput(&format_, input_callback, this, nullptr, nullptr, 0, &queue_) != noErr) {
                throw MediaDeviceError("Failed to create input audio queue");
            }
            set_queue_device();
            for (auto& buffer : buffers_) {
                if (AudioQueueAllocateBuffer(queue_, buffer_bytes_, &buffer) != noErr) {
                    throw MediaDeviceError("Failed to allocate audio queue buffer");
                }
                AudioQueueEnqueueBuffer(queue_, buffer, 0, nullptr);
            }
        } else {
            if (AudioQueueNewOutput(&format_, output_callback, this, nullptr, nullptr, 0, &queue_) != noErr) {
                throw MediaDeviceError("Failed to create output audio queue");
            }
            set_queue_device();
            for (auto& buffer : buffers_) {
                if (AudioQueueAllocateBuffer(queue_, buffer_bytes_, &buffer) != noErr) {
                    throw MediaDeviceError("Failed to allocate audio queue buffer");
                }
                buffer->mAudioDataByteSize = buffer_bytes_;
                std::memset(buffer->mAudioData, 0, buffer_bytes_);
                AudioQueueEnqueueBuffer(queue_, buffer, 0, nullptr);
            }
        }
        if (AudioQueueStart(queue_, nullptr) != noErr) {
            throw MediaDeviceError("Failed to start audio queue");
        }
    }

    void MacAudioDeviceModule::input_callback(void* user_data, AudioQueueRef, AudioQueueBufferRef buffer, const AudioTimeStamp*, UInt32, const AudioStreamPacketDescription*) {
        static_cast<MacAudioDeviceModule*>(user_data)->handle_input(buffer);
    }

    void MacAudioDeviceModule::output_callback(void* user_data, AudioQueueRef, AudioQueueBufferRef buffer) {
        static_cast<MacAudioDeviceModule*>(user_data)->handle_output(buffer);
    }

    void MacAudioDeviceModule::handle_input(AudioQueueBufferRef buffer) const {
        if (!running_) {
            return;
        }
        if (buffer->mAudioDataByteSize > 0) {
            auto data = bytes::make_unique_binary(buffer->mAudioDataByteSize);
            std::memcpy(data.get(), buffer->mAudioData, buffer->mAudioDataByteSize);
            data_callback_(std::move(data), {});
        }
        AudioQueueEnqueueBuffer(queue_, buffer, 0, nullptr);
    }

    void MacAudioDeviceModule::handle_output(AudioQueueBufferRef buffer) {
        if (!running_) {
            return;
        }
        {
            const std::lock_guard lock(queue_mutex_);
            if (!data_queue_.empty()) {
                std::memcpy(buffer->mAudioData, data_queue_.front().get(), buffer_bytes_);
                data_queue_.pop();
            } else {
                std::memset(buffer->mAudioData, 0, buffer_bytes_);
            }
        }
        buffer->mAudioDataByteSize = buffer_bytes_;
        AudioQueueEnqueueBuffer(queue_, buffer, 0, nullptr);
    }

    void MacAudioDeviceModule::on_data(bytes::unique_binary data) {
        if (!running_) {
            return;
        }
        const std::lock_guard lock(queue_mutex_);
        data_queue_.emplace(std::move(data));
    }
} // ntgcalls::media::devices

#endif
