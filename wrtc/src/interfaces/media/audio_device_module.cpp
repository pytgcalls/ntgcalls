//
// Created by Laky64 on 04/10/24.
//

#include <wrtc/interfaces/media/audio_device_module.hpp>

namespace wrtc {
    int32_t AudioDeviceModule::ActiveAudioLayer(AudioLayer* audioLayer) const {
        *audioLayer = kDummyAudio;
        return 0;
    }

    int32_t AudioDeviceModule::RegisterAudioCallback(webrtc::AudioTransport* audioCallback) {
        webrtc::MutexLock lock(&mutex);
        this->audioCallback = audioCallback;
        return 0;
    }

    int32_t AudioDeviceModule::Init() {
        return 0;
    }

    int32_t AudioDeviceModule::Terminate() {
        return -1;
    }

    bool AudioDeviceModule::Initialized() const {
        return false;
    }

    int16_t AudioDeviceModule::PlayoutDevices() {
        return -1;
    }

    int16_t AudioDeviceModule::RecordingDevices() {
        return -1;
    }

    int32_t AudioDeviceModule::PlayoutDeviceName(uint16_t index, char name[128], char guid[128]) {
        return -1;
    }

    int32_t AudioDeviceModule::RecordingDeviceName(uint16_t index, char name[128], char guid[128]) {
        return -1;
    }

    int32_t AudioDeviceModule::SetPlayoutDevice(uint16_t index) {
        return -1;
    }

    int32_t AudioDeviceModule::SetPlayoutDevice(WindowsDeviceType device) {
        return -1;
    }

    int32_t AudioDeviceModule::SetRecordingDevice(uint16_t index) {
        return -1;
    }

    int32_t AudioDeviceModule::SetRecordingDevice(WindowsDeviceType device) {
        return -1;
    }

    int32_t AudioDeviceModule::PlayoutIsAvailable(bool* available) {
        *available = true;
        return 0;
    }

    int32_t AudioDeviceModule::InitPlayout() {
        playIsInitialized = true;
        return 0;
    }

    bool AudioDeviceModule::PlayoutIsInitialized() const {
        return playIsInitialized;
    }

    int32_t AudioDeviceModule::RecordingIsAvailable(bool* available) {
        return -1;
    }

    int32_t AudioDeviceModule::InitRecording() {
        return -1;
    }

    bool AudioDeviceModule::RecordingIsInitialized() const {
        return false;
    }

    int32_t AudioDeviceModule::StartPlayout() {
        if (!playIsInitialized) {
            return -1;
        }
        {
            webrtc::MutexLock lock(&mutex);
            playing = true;
        }
        UpdateProcessing(true);
        return 0;
    }

    int32_t AudioDeviceModule::StopPlayout() {
        {
            webrtc::MutexLock lock(&mutex);
            playing = false;
        }
        UpdateProcessing(false);
        return 0;
    }

    bool AudioDeviceModule::Playing() const {
        return playing;
    }

    int32_t AudioDeviceModule::StartRecording() {
        return -1;
    }

    int32_t AudioDeviceModule::StopRecording() {
        return -1;
    }

    bool AudioDeviceModule::Recording() const {
        return false;
    }

    int32_t AudioDeviceModule::InitSpeaker() {
        return 0;
    }

    bool AudioDeviceModule::SpeakerIsInitialized() const {
        return true;
    }

    int32_t AudioDeviceModule::InitMicrophone() {
        return -1;
    }

    bool AudioDeviceModule::MicrophoneIsInitialized() const {
        return false;
    }

    int32_t AudioDeviceModule::SpeakerVolumeIsAvailable(bool* available) {
        return -1;
    }

    int32_t AudioDeviceModule::SetSpeakerVolume(uint32_t volume) {
        return -1;
    }

    int32_t AudioDeviceModule::SpeakerVolume(uint32_t* volume) const {
        return -1;
    }

    int32_t AudioDeviceModule::MaxSpeakerVolume(uint32_t* maxVolume) const {
        return -1;
    }

    int32_t AudioDeviceModule::MinSpeakerVolume(uint32_t* minVolume) const {
        return -1;
    }

    int32_t AudioDeviceModule::MicrophoneVolumeIsAvailable(bool* available) {
        return -1;
    }

    int32_t AudioDeviceModule::SetMicrophoneVolume(const uint32_t volume) {
        return -1;
    }

    int32_t AudioDeviceModule::MicrophoneVolume(uint32_t* volume) const {
        return -1;
    }

    int32_t AudioDeviceModule::MaxMicrophoneVolume(uint32_t* maxVolume) const {
        return -1;
    }

    int32_t AudioDeviceModule::MinMicrophoneVolume(uint32_t* minVolume) const {
        return -1;
    }

    int32_t AudioDeviceModule::SpeakerMuteIsAvailable(bool* available) {
        return false;
    }

    int32_t AudioDeviceModule::SetSpeakerMute(bool enable) {
        return -1;
    }

    int32_t AudioDeviceModule::SpeakerMute(bool* enabled) const {
        return -1;
    }

    int32_t AudioDeviceModule::MicrophoneMuteIsAvailable(bool* available) {
        return -1;
    }

    int32_t AudioDeviceModule::SetMicrophoneMute(bool enable) {
        return -1;
    }

    int32_t AudioDeviceModule::MicrophoneMute(bool* enabled) const {
        return -1;
    }

    int32_t AudioDeviceModule::StereoPlayoutIsAvailable(bool* available) const {
        *available = true;
        return 0;
    }

    int32_t AudioDeviceModule::SetStereoPlayout(bool enable) {
        return 0;
    }

    int32_t AudioDeviceModule::StereoPlayout(bool* enabled) const {
        *enabled = true;
        return 0;
    }

    int32_t AudioDeviceModule::StereoRecordingIsAvailable(bool* available) const {
        return -1;
    }

    int32_t AudioDeviceModule::SetStereoRecording(const bool enable) {
        return -1;
    }

    int32_t AudioDeviceModule::StereoRecording(bool* enabled) const {
        return -1;
    }

    int32_t AudioDeviceModule::PlayoutDelay(uint16_t* delayMS) const {
        *delayMS = 0;
        return 0;
    }

    bool AudioDeviceModule::BuiltInAECIsAvailable() const {
        return false;
    }

    bool AudioDeviceModule::BuiltInAGCIsAvailable() const {
        return false;
    }

    bool AudioDeviceModule::BuiltInNSIsAvailable() const {
        return false;
    }

    int32_t AudioDeviceModule::EnableBuiltInAEC(bool enable) {
        return -1;
    }

    int32_t AudioDeviceModule::EnableBuiltInAGC(bool enable) {
        return -1;
    }

    int32_t AudioDeviceModule::EnableBuiltInNS(bool enable) {
        return -1;
    }

    void AudioDeviceModule::UpdateProcessing(const bool start) {
        if (start) {
            if (!processThread) {
                processThread = rtc::Thread::Create();
                processThread->Start();
            }
            processThread->PostTask([this] { StartProcessP(); });
        } else {
            if (processThread) {
                processThread->Stop();
                processThread.reset(nullptr);
                processThreadChecker.Detach();
            }
            webrtc::MutexLock lock(&mutex);
            started = false;
        }
    }

    void AudioDeviceModule::StartProcessP() {
        RTC_DCHECK_RUN_ON(&processThreadChecker);
        {
            webrtc::MutexLock lock(&mutex);
            if (started) {
                return;
            }
        }
        ProcessFrameP();
    }

    void AudioDeviceModule::ProcessFrameP() {
        RTC_DCHECK_RUN_ON(&processThreadChecker);
        {
            webrtc::MutexLock lock(&mutex);
            if (!started) {
                nextFrameTime = rtc::TimeMillis();
                started = true;
            }

            if (playing) {
                ReceiveFrameP();
            }
        }

        nextFrameTime += 10;
        const int64_t current_time = rtc::TimeMillis();
        const int64_t wait_time = nextFrameTime > current_time ? nextFrameTime - current_time : 0;
        processThread->PostDelayedTask([this] { ProcessFrameP(); },webrtc::TimeDelta::Millis(wait_time));
    }

    void AudioDeviceModule::ReceiveFrameP() {
        RTC_DCHECK_RUN_ON(&processThreadChecker);
        if (!audioCallback) {
            return;
        }
        memset(buffer, 0, sizeof(buffer));
        size_t nSamplesOut = 0;
        int64_t elapsedTimeMs = 0;
        int64_t ntpTimeMs = 0;
        audioCallback->NeedMorePlayData(
            kNumberSamples,
            kNumberBytesPerSample,
            kNumberOfChannels,
            kSamplesPerSecond,
            buffer,
            nSamplesOut,
            &elapsedTimeMs,
            &ntpTimeMs
        );
    }
} // wrtc