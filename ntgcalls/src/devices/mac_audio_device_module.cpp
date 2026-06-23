//
// Created by Laky64 on 23/06/2026.
//

#include <ntgcalls/devices/mac_audio_device_module.hpp>

#ifdef IS_MACOS

#include <cstring>
#include <vector>
#include <ntgcalls/exceptions.hpp>

#ifndef kAudioObjectPropertyElementMain
#define kAudioObjectPropertyElementMain kAudioObjectPropertyElementMaster
#endif

namespace ntgcalls {
    MacAudioDeviceModule::MacAudioDeviceModule(const AudioDescription* desc, const bool isCapture, BaseSink *sink):
        BaseIO(sink), BaseDeviceModule(desc, isCapture), BaseReader(sink), AudioMixer(sink) {
        try {
            deviceUID = deviceMetadata["uid"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        format.mSampleRate = rate;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
        format.mBitsPerChannel = 16;
        format.mChannelsPerFrame = channels;
        format.mFramesPerPacket = 1;
        format.mBytesPerFrame = format.mChannelsPerFrame * (format.mBitsPerChannel / 8);
        format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;
    }

    MacAudioDeviceModule::~MacAudioDeviceModule() {
        running = false;
        if (queue) {
            AudioQueueStop(queue, true);
            AudioQueueDispose(queue, true);
            queue = nullptr;
        }
    }

    bool MacAudioDeviceModule::isSupported() {
        return true;
    }

    std::vector<DeviceInfo> MacAudioDeviceModule::getDevices() {
        std::vector<DeviceInfo> devices;
        AudioObjectPropertyAddress devicesAddr = {
            kAudioHardwarePropertyDevices,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMain
        };
        UInt32 dataSize = 0;
        if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &devicesAddr, 0, nullptr, &dataSize) != noErr || dataSize == 0) {
            return devices;
        }
        std::vector<AudioDeviceID> ids(dataSize / sizeof(AudioDeviceID));
        if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &devicesAddr, 0, nullptr, &dataSize, ids.data()) != noErr) {
            return devices;
        }

        for (const auto id : ids) {
            const auto uid = stringProperty(id, kAudioDevicePropertyDeviceUID);
            if (uid.empty()) {
                continue;
            }
            const auto name = stringProperty(id, kAudioObjectPropertyName);
            if (channelsForScope(id, kAudioObjectPropertyScopeInput) > 0) {
                const json metadata = {
                    {"is_microphone", true},
                    {"uid", uid},
                };
                devices.emplace_back(name, metadata.dump());
            }
            if (channelsForScope(id, kAudioObjectPropertyScopeOutput) > 0) {
                const json metadata = {
                    {"is_microphone", false},
                    {"uid", uid},
                };
                devices.emplace_back(name, metadata.dump());
            }
        }
        return devices;
    }

    std::string MacAudioDeviceModule::cfStringToStd(CFStringRef value) {
        if (!value) {
            return {};
        }
        const CFIndex length = CFStringGetLength(value);
        const CFIndex maxBytes = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        std::string out(maxBytes, '\0');
        if (CFStringGetCString(value, out.data(), maxBytes, kCFStringEncodingUTF8)) {
            out.resize(std::strlen(out.c_str()));
            return out;
        }
        return {};
    }

    std::string MacAudioDeviceModule::stringProperty(const AudioDeviceID id, const AudioObjectPropertySelector selector) {
        AudioObjectPropertyAddress addr = { selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
        CFStringRef value = nullptr;
        UInt32 size = sizeof(value);
        if (AudioObjectGetPropertyData(id, &addr, 0, nullptr, &size, &value) != noErr || !value) {
            return {};
        }
        auto result = cfStringToStd(value);
        CFRelease(value);
        return result;
    }

    UInt32 MacAudioDeviceModule::channelsForScope(const AudioDeviceID id, const AudioObjectPropertyScope scope) {
        AudioObjectPropertyAddress addr = { kAudioDevicePropertyStreamConfiguration, scope, kAudioObjectPropertyElementMain };
        UInt32 size = 0;
        if (AudioObjectGetPropertyDataSize(id, &addr, 0, nullptr, &size) != noErr || size == 0) {
            return 0;
        }
        std::vector<uint8_t> storage(size);
        const auto bufferList = reinterpret_cast<AudioBufferList*>(storage.data());
        if (AudioObjectGetPropertyData(id, &addr, 0, nullptr, &size, bufferList) != noErr) {
            return 0;
        }
        UInt32 count = 0;
        for (UInt32 i = 0; i < bufferList->mNumberBuffers; i++) {
            count += bufferList->mBuffers[i].mNumberChannels;
        }
        return count;
    }

    void MacAudioDeviceModule::setQueueDevice() const {
        if (deviceUID.empty()) {
            return;
        }
        CFStringRef uidRef = CFStringCreateWithCString(nullptr, deviceUID.c_str(), kCFStringEncodingUTF8);
        if (!uidRef) {
            return;
        }
        AudioQueueSetProperty(queue, kAudioQueueProperty_CurrentDevice, &uidRef, sizeof(uidRef));
        CFRelease(uidRef);
    }

    void MacAudioDeviceModule::open() {
        if (running) return;
        bufferBytes = static_cast<uint32_t>(sink->frameSize());
        if (bufferBytes == 0) {
            throw MediaDeviceError("Invalid audio frame size");
        }
        running = true;
        if (isCapture) {
            if (AudioQueueNewInput(&format, inputCallback, this, nullptr, nullptr, 0, &queue) != noErr) {
                throw MediaDeviceError("Failed to create input audio queue");
            }
            setQueueDevice();
            for (auto& buffer : buffers) {
                if (AudioQueueAllocateBuffer(queue, bufferBytes, &buffer) != noErr) {
                    throw MediaDeviceError("Failed to allocate audio queue buffer");
                }
                AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
            }
        } else {
            if (AudioQueueNewOutput(&format, outputCallback, this, nullptr, nullptr, 0, &queue) != noErr) {
                throw MediaDeviceError("Failed to create output audio queue");
            }
            setQueueDevice();
            for (auto& buffer : buffers) {
                if (AudioQueueAllocateBuffer(queue, bufferBytes, &buffer) != noErr) {
                    throw MediaDeviceError("Failed to allocate audio queue buffer");
                }
                buffer->mAudioDataByteSize = bufferBytes;
                std::memset(buffer->mAudioData, 0, bufferBytes);
                AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
            }
        }
        if (AudioQueueStart(queue, nullptr) != noErr) {
            throw MediaDeviceError("Failed to start audio queue");
        }
    }

    void MacAudioDeviceModule::inputCallback(void* userData, AudioQueueRef, const AudioQueueBufferRef buffer, const AudioTimeStamp*, UInt32, const AudioStreamPacketDescription*) {
        static_cast<MacAudioDeviceModule*>(userData)->handleInput(buffer);
    }

    void MacAudioDeviceModule::outputCallback(void* userData, AudioQueueRef, const AudioQueueBufferRef buffer) {
        static_cast<MacAudioDeviceModule*>(userData)->handleOutput(buffer);
    }

    void MacAudioDeviceModule::handleInput(const AudioQueueBufferRef buffer) {
        if (!running) {
            return;
        }
        if (buffer->mAudioDataByteSize > 0) {
            auto data = bytes::make_unique_binary(buffer->mAudioDataByteSize);
            std::memcpy(data.get(), buffer->mAudioData, buffer->mAudioDataByteSize);
            dataCallback(std::move(data), {});
        }
        AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    }

    void MacAudioDeviceModule::handleOutput(const AudioQueueBufferRef buffer) {
        if (!running) {
            return;
        }
        {
            std::lock_guard lock(queueMutex);
            if (!dataQueue.empty()) {
                std::memcpy(buffer->mAudioData, dataQueue.front().get(), bufferBytes);
                dataQueue.pop();
            } else {
                std::memset(buffer->mAudioData, 0, bufferBytes);
            }
        }
        buffer->mAudioDataByteSize = bufferBytes;
        AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    }

    void MacAudioDeviceModule::onData(bytes::unique_binary data) {
        if (!running) {
            return;
        }
        std::lock_guard lock(queueMutex);
        dataQueue.emplace(std::move(data));
    }
} // ntgcalls

#endif