//
// Created by Laky64 on 04/10/24.
//

#pragma once

#define WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE
#include <modules/audio_device/audio_device_impl.h>
#include <rtc_base/thread.h>

namespace wrtc {
    typedef uint16_t Sample;
    static constexpr uint8_t kNumberOfChannels = 2;
    static constexpr int kSamplesPerSecond = 96000;
    static constexpr size_t kNumberSamples = kSamplesPerSecond / 100;
    static constexpr size_t kNumberBytesPerSample = sizeof(Sample);

    class AudioDeviceModule: public webrtc::AudioDeviceModule {
        bool playIsInitialized = false;
        bool playing = false;
        mutable webrtc::Mutex mutex;
        webrtc::AudioTransport* audioCallback RTC_GUARDED_BY(mutex) = nullptr;
        bool started RTC_GUARDED_BY(mutex) = false;
        std::unique_ptr<rtc::Thread> processThread;
        webrtc::SequenceChecker processThreadChecker{webrtc::SequenceChecker::kDetached};
        int64_t nextFrameTime RTC_GUARDED_BY(processThreadChecker) = 0;
        char buffer[kNumberSamples * kNumberBytesPerSample * kNumberOfChannels]{};

        void UpdateProcessing(bool start) RTC_LOCKS_EXCLUDED(mutex);

        void StartProcessP();

        void ProcessFrameP();

        void ReceiveFrameP();

    public:
        int32_t ActiveAudioLayer(AudioLayer* audioLayer) const override;

        int32_t RegisterAudioCallback(webrtc::AudioTransport* audioCallback) override;

        int32_t Init() override;

        int32_t Terminate() override;

        [[nodiscard]] bool Initialized() const override;

        int16_t PlayoutDevices() override;

        int16_t RecordingDevices() override;

        int32_t PlayoutDeviceName(uint16_t index, char name[128], char guid[128]) override;

        int32_t RecordingDeviceName(uint16_t index, char name[128], char guid[128]) override;

        int32_t SetPlayoutDevice(uint16_t index) override;

        int32_t SetPlayoutDevice(WindowsDeviceType device) override;

        int32_t SetRecordingDevice(uint16_t index) override;

        int32_t SetRecordingDevice(WindowsDeviceType device) override;

        int32_t PlayoutIsAvailable(bool* available) override;

        int32_t InitPlayout() override;

        [[nodiscard]] bool PlayoutIsInitialized() const override;

        int32_t RecordingIsAvailable(bool* available) override;

        int32_t InitRecording() override;

        [[nodiscard]] bool RecordingIsInitialized() const override;

        int32_t StartPlayout() override;

        int32_t StopPlayout() override;

        [[nodiscard]] bool Playing() const override;

        int32_t StartRecording() override;

        int32_t StopRecording() override;

        bool Recording() const override;

        int32_t InitSpeaker() override;

        bool SpeakerIsInitialized() const override;

        int32_t InitMicrophone() override;

        bool MicrophoneIsInitialized() const override;

        int32_t SpeakerVolumeIsAvailable(bool* available) override;

        int32_t SetSpeakerVolume(uint32_t volume) override;

        int32_t SpeakerVolume(uint32_t* volume) const override;

        int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override;

        int32_t MinSpeakerVolume(uint32_t* minVolume) const override;

        int32_t MicrophoneVolumeIsAvailable(bool* available) override;

        int32_t SetMicrophoneVolume(uint32_t volume) override;

        int32_t MicrophoneVolume(uint32_t* volume) const override;

        int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override;

        int32_t MinMicrophoneVolume(uint32_t* minVolume) const override;

        int32_t SpeakerMuteIsAvailable(bool* available) override;

        int32_t SetSpeakerMute(bool enable) override;

        int32_t SpeakerMute(bool* enabled) const override;

        int32_t MicrophoneMuteIsAvailable(bool* available) override;

        int32_t SetMicrophoneMute(bool enable) override;

        int32_t MicrophoneMute(bool* enabled) const override;

        int32_t StereoPlayoutIsAvailable(bool* available) const override;

        int32_t SetStereoPlayout(bool enable) override;

        int32_t StereoPlayout(bool* enabled) const override;

        int32_t StereoRecordingIsAvailable(bool* available) const override;

        int32_t SetStereoRecording(bool enable) override;

        int32_t StereoRecording(bool* enabled) const override;

        int32_t PlayoutDelay(uint16_t* delayMS) const override;

        bool BuiltInAECIsAvailable() const override;

        bool BuiltInAGCIsAvailable() const override;

        bool BuiltInNSIsAvailable() const override;

        int32_t EnableBuiltInAEC(bool enable) override;

        int32_t EnableBuiltInAGC(bool enable) override;

        int32_t EnableBuiltInNS(bool enable) override;
    };

} // wrtc
