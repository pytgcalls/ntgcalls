//
// Created by Lauren on 04/10/24.
//

#include <wrtc/interfaces/media/audio_device_module.hpp>

namespace wrtc::interfaces::media {
    int32_t AudioDeviceModule::ActiveAudioLayer(AudioLayer* audio_layer) const {
        *audio_layer = kDummyAudio;
        return 0;
    }

    int32_t AudioDeviceModule::RegisterAudioCallback(webrtc::AudioTransport* callback) {
        const webrtc::MutexLock lock(&mutex_);
        audio_callback_ = callback;
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
        play_is_initialized_ = true;
        return 0;
    }

    bool AudioDeviceModule::PlayoutIsInitialized() const {
        return play_is_initialized_;
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
        if (!play_is_initialized_) {
            return -1;
        }
        {
            const webrtc::MutexLock lock(&mutex_);
            playing_ = true;
        }
        update_processing(true);
        return 0;
    }

    int32_t AudioDeviceModule::StopPlayout() {
        {
            const webrtc::MutexLock lock(&mutex_);
            playing_ = false;
        }
        update_processing(false);
        return 0;
    }

    bool AudioDeviceModule::Playing() const {
        return playing_;
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

    int32_t AudioDeviceModule::MaxSpeakerVolume(uint32_t* max_volume) const {
        return -1;
    }

    int32_t AudioDeviceModule::MinSpeakerVolume(uint32_t* min_volume) const {
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

    int32_t AudioDeviceModule::MaxMicrophoneVolume(uint32_t* max_volume) const {
        return -1;
    }

    int32_t AudioDeviceModule::MinMicrophoneVolume(uint32_t* min_volume) const {
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

    int32_t AudioDeviceModule::PlayoutDelay(uint16_t* delay_ms) const {
        *delay_ms = 0;
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

    void AudioDeviceModule::update_processing(const bool start) {
        if (start) {
            if (!process_thread_) {
                process_thread_ = utils::SafeThread::Create();
                process_thread_->Start();
            }
            process_thread_->PostTask([this] { start_process_p(); });
        } else {
            if (process_thread_) {
                process_thread_->Stop();
                process_thread_.reset(nullptr);
                process_thread_checker_.Detach();
            }
            const webrtc::MutexLock lock(&mutex_);
            started_ = false;
        }
    }

    void AudioDeviceModule::start_process_p() {
        RTC_DCHECK_RUN_ON(&process_thread_checker_);
        {
            const webrtc::MutexLock lock(&mutex_);
            if (started_) {
                return;
            }
        }
        process_frame_p();
    }

    void AudioDeviceModule::process_frame_p() {
        RTC_DCHECK_RUN_ON(&process_thread_checker_);
        {
            const webrtc::MutexLock lock(&mutex_);
            if (!started_) {
                next_frame_time_ = webrtc::TimeMillis();
                started_ = true;
            }

            if (playing_) {
                receive_frame_p();
            }
        }

        next_frame_time_ += 10;
        const int64_t current_time = webrtc::TimeMillis();
        const int64_t wait_time = next_frame_time_ > current_time ? next_frame_time_ - current_time : 0;
        process_thread_->PostDelayedTask([this] { process_frame_p(); }, webrtc::TimeDelta::Millis(wait_time));
    }

    void AudioDeviceModule::receive_frame_p() {
        RTC_DCHECK_RUN_ON(&process_thread_checker_);
        if (!audio_callback_) {
            return;
        }
        std::memset(buffer_, 0, sizeof(buffer_));
        size_t n_samples_out = 0;
        int64_t elapsed_time_ms = 0;
        int64_t ntp_time_ms = 0;
        audio_callback_->NeedMorePlayData(
            kNumberSamples,
            kNumberBytesPerSample,
            kNumberOfChannels,
            kSamplesPerSecond,
            buffer_,
            n_samples_out,
            &elapsed_time_ms,
            &ntp_time_ms
        );
    }
} // wrtc::interfaces::media
