//
// Created by Laky64 on 20/09/2024.
//

#include <ntgcalls/devices/win_core_device_module.hpp>
#include <ntgcalls/media/audio_sink.hpp>

#ifdef IS_WINDOWS

#include <ntgcalls/exceptions.hpp>
#include <cmath>

namespace ntgcalls {

    WinCoreDeviceModule::WinCoreDeviceModule(const AudioDescription* desc, const bool isCapture, BaseSink *sink):
        BaseIO(sink),
        BaseDeviceModule(desc, isCapture),
        BaseReader(sink),
        AudioMixer(sink),
        comInitializer(webrtc::ScopedCOMInitializer::kMTA),
        mmcssRegistration(L"Pro Audio")
    {
        RTC_DCHECK(comInitializer.Succeeded());
        RTC_DCHECK(mmcssRegistration.Succeeded());

        audioSamplesEvent.Set(CreateEvent(nullptr, false, false, nullptr));
        RTC_DCHECK(audioSamplesEvent.IsValid());
        restartEvent.Set(CreateEvent(nullptr, false, false, nullptr));
        RTC_DCHECK(restartEvent.IsValid());
        stopEvent.Set(CreateEvent(nullptr, false, false, nullptr));
        RTC_DCHECK(stopEvent.IsValid());
        try {
            deviceIndex = deviceMetadata["index"];
            deviceUID = deviceMetadata["uid"];
            automaticRestart = deviceMetadata["automatic_restart"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        if (deviceIndex < 0) {
            throw MediaDeviceError("Invalid device index");
        }
    }

    WinCoreDeviceModule::~WinCoreDeviceModule() {
        std::lock_guard queueLock(queueMutex);
        SetEvent(stopEvent.Get());
        if (isCapture) {
            thread.Finalize();
        }
        ResetEvent(stopEvent.Get());
        ResetEvent(restartEvent.Get());
        ResetEvent(audioSamplesEvent.Get());
        stop();
    }

    bool WinCoreDeviceModule::isSupported() {
        return core_audio_utility::IsMMCSSSupported();
    }

    std::vector<DeviceInfo> WinCoreDeviceModule::getDevices() {
        auto appendDevices = [](const webrtc::AudioDeviceNames& deviceNames, std::vector<DeviceInfo>& devices, const bool isMicrophone = false) {
            int index = 0;
            for (const auto& device : deviceNames) {
                json metadata = {
                    {"is_microphone", isMicrophone},
                    {"index", index},
                    {"uid", device.unique_id},
                    {"automatic_restart", true}
                };
                devices.emplace_back(device.device_name, metadata.dump());
                index++;
            }
        };
        webrtc::ScopedCOMInitializer comInitializer(webrtc::ScopedCOMInitializer::kMTA);
        webrtc::AudioDeviceNames deviceNames;
        std::vector<DeviceInfo> devices;
        core_audio_utility::GetInputDeviceNames(&deviceNames);
        appendDevices(deviceNames, devices, true);
        deviceNames.clear();
        core_audio_utility::GetOutputDeviceNames(&deviceNames);
        appendDevices(deviceNames, devices);
        return devices;
    }

    void WinCoreDeviceModule::open() {
        init();
        runDataListener();
    }

    void WinCoreDeviceModule::init() {
        if (running) return;
        running = true;
        const auto dataFlow = isCapture ? eCapture:eRender;
        std::string deviceId = webrtc::AudioDeviceName::kDefaultDeviceId;
        auto role = ERole();
        switch (deviceIndex) {
        case 0:
            role = eConsole;
            break;
        case 1:
            role = eCommunications;
            break;
        default:
            deviceId = deviceUID;
            break;
        }

        const auto audioClientVersion = core_audio_utility::GetAudioClientVersion();
        switch (audioClientVersion) {
        case 3:
            RTC_LOG(LS_INFO) << "Using CoreAudioV3";
            audioClient = core_audio_utility::CreateClient3(deviceId, dataFlow, role);
            break;
        case 2:
            RTC_LOG(LS_INFO) << "Using CoreAudioV2";
            audioClient = core_audio_utility::CreateClient2(deviceId, dataFlow, role);
            break;
        default:
            RTC_LOG(LS_ERROR) << "Using CoreAudioV1";
            audioClient = core_audio_utility::CreateClient(deviceId, dataFlow, role);
        }
        if (!audioClient) {
            throw MediaDeviceError("Failed to create audio client");
        }
        if (audioClientVersion >= 2) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            if (FAILED(core_audio_utility::SetClientProperties(static_cast<IAudioClient2*>(audioClient.Get())))) {
                throw MediaDeviceError("Failed to set client properties");
            }
        }

        webrtc::AudioParameters params;
        if (FAILED(core_audio_utility::GetPreferredAudioParameters(audioClient.Get(), &params))) {
            throw MediaDeviceError("Failed to get preferred audio parameters");
        }

        WAVEFORMATEX* f = &format.Format;
        f->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        f->nChannels = rtc::dchecked_cast<WORD>(params.channels());
        f->nSamplesPerSec = rtc::dchecked_cast<DWORD>(params.sample_rate());
        f->wBitsPerSample = rtc::dchecked_cast<WORD>(params.bits_per_sample());
        f->nBlockAlign = f->wBitsPerSample / 8 * f->nChannels;
        f->nAvgBytesPerSec = f->nSamplesPerSec * f->nBlockAlign;
        f->cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        format.Samples.wValidBitsPerSample = rtc::dchecked_cast<WORD>(params.bits_per_sample());
        format.dwChannelMask = f->nChannels == 1 ? KSAUDIO_SPEAKER_MONO : KSAUDIO_SPEAKER_STEREO;
        format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        rate = params.sample_rate();
        channels = params.channels();

        const auto audioSink = dynamic_cast<AudioSink*>(sink);
        if (auto config = audioSink->getConfig(); config->channelCount != channels || config->sampleRate != rate) {
            RTC_LOG(LS_INFO) << "Updating Audio Configuration...";
            config->channelCount = channels;
            config->sampleRate = rate;
            dynamic_cast<AudioSink*>(sink) -> setConfig(config);
        }

        // TODO: Low latency mode is not supported yet
        if (FAILED(core_audio_utility::SharedModeInitialize(audioClient.Get(), &format, audioSamplesEvent, 0, true, &endpointBufferSizeFrames))) {
            throw MediaDeviceError("Failed to initialize shared mode");
        }

        if (!core_audio_utility::IsFormatSupported(audioClient.Get(), AUDCLNT_SHAREMODE_SHARED, &format)) {
            throw MediaDeviceError("Unsupported audio format with " + std::to_string(channels) + " channels");
        }

        REFERENCE_TIME device_period;
        if (FAILED(core_audio_utility::GetDevicePeriod(audioClient.Get(), AUDCLNT_SHAREMODE_SHARED, &device_period))) {
            throw MediaDeviceError("Failed to get device period");
        }

        const double device_period_in_seconds = static_cast<double>(core_audio_utility::ReferenceTimeToTimeDelta(device_period).ms()) / 1000.0;
        if (const int preferred_frames_per_buffer = static_cast<int>(lround(params.sample_rate() * device_period_in_seconds)); preferred_frames_per_buffer % params.frames_per_buffer()) {
            RTC_LOG(LS_WARNING) << "Preferred frames per buffer is not a multiple of frames per buffer";
        }

        audioSessionControl = core_audio_utility::CreateAudioSessionControl(audioClient.Get());
        if (!audioSessionControl.Get()) {
            throw MediaDeviceError("Failed to create audio session control");
        }

        AudioSessionState state;
        if (FAILED(audioSessionControl->GetState(&state))) {
            throw MediaDeviceError("Failed to get audio session state");
        }

        if (FAILED(audioSessionControl->RegisterAudioSessionNotification(this))) {
            throw MediaDeviceError("Failed to register audio session notification");
        }

        if (isCapture) {
            audioCaptureClient = core_audio_utility::CreateCaptureClient(audioClient.Get());
        } else {
            audioRenderClient = core_audio_utility::CreateRenderClient(audioClient.Get());
            core_audio_utility::FillRenderEndpointBufferWithSilence(audioClient.Get(), audioRenderClient.Get());
        }
        if (FAILED(static_cast<_com_error>(audioClient->Start()).Error())) {
            throw MediaDeviceError("Failed to start audio client");
        }
    }

    void WinCoreDeviceModule::releaseCOMObjects() {
        if (audioRenderClient.Get()) {
            audioRenderClient.Reset();
        }
        if (audioCaptureClient.Get()) {
            audioCaptureClient.Reset();
        }
        if (audioRenderClient.Get()) {
            audioRenderClient.Reset();
        }
        if (audioClient) {
            audioClient.Reset();
        }
        if (audioSessionControl.Get()) {
            audioSessionControl.Reset();
        }
    }

    HRESULT WinCoreDeviceModule::QueryInterface(const IID& riid, void** ppvObject) {
        if (ppvObject == nullptr) {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == __uuidof(IAudioSessionEvents)) {
            *ppvObject = static_cast<IAudioSessionEvents*>(this);
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WinCoreDeviceModule::AddRef() {
        return InterlockedIncrement(&refCount);
    }

    ULONG WinCoreDeviceModule::Release() {
        return InterlockedDecrement(&refCount);
    }

    HRESULT WinCoreDeviceModule::OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[], DWORD ChangedChannel, LPCGUID EventContext) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnStateChanged(AudioSessionState NewState) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnSessionDisconnected(const AudioSessionDisconnectReason DisconnectReason) {
        if (!automaticRestart) {
            return S_OK;
        }
        if (isRestarting) {
            return S_OK;
        }
        if (DisconnectReason == DisconnectReasonDeviceRemoval || DisconnectReason == DisconnectReasonFormatChanged) {
            isRestarting = true;
            SetEvent(restartEvent.Get());
        }
        return S_OK;
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    bool WinCoreDeviceModule::handleRestartEvent() {
        bool restartOk = true;
        try {
            stop();
            switchDevice();
            init();
        } catch (...) {
            restartOk = false;
        }
        isRestarting = false;
        return restartOk;
    }

    void WinCoreDeviceModule::runDataListener() {
        thread = rtc::PlatformThread::SpawnJoinable(
            [this] {
                bool streaming = true;
                bool error = false;
                HANDLE waitArray[] = {stopEvent.Get(), restartEvent.Get(), audioSamplesEvent.Get()};
                while (streaming && !error) {
                    switch (WaitForMultipleObjects(arraysize(waitArray), waitArray, false, INFINITE)) {
                    case WAIT_OBJECT_0 + 0:
                        streaming = false;
                        break;
                    case WAIT_OBJECT_0 + 1:
                        error = !handleRestartEvent();
                        break;
                    case WAIT_OBJECT_0 + 2:
                        if (isCapture) {
                            error = !handleDataRecord();
                        } else {
                            error = !handleDataPlayback();
                        }
                        break;
                    default:
                        error = true;
                        break;
                    }
                }
                if (streaming && error) {
                    if (const _com_error result = audioClient->Stop(); FAILED(result.Error())) {
                        RTC_LOG(LS_ERROR) << "IAudioClient::Stop failed: " << core_audio_utility::ErrorToString(result);
                    }
                }
            },
            "WinCoreAudio",
            rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime)
        );
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    bool WinCoreDeviceModule::handleDataRecord() const {
        if (!running) {
            return false;
        }
        UINT32 numFramesInNextPacket = 0;
        _com_error error = audioCaptureClient->GetNextPacketSize(&numFramesInNextPacket);
        if (error.Error() == AUDCLNT_E_DEVICE_INVALIDATED) {
            return true;
        }
        if (FAILED(error.Error())) {
            return false;
        }
        while (numFramesInNextPacket > 0) {
            uint8_t* audioData;
            UINT32 numFramesToRead = 0;
            DWORD flags = 0;
            UINT64 devicePositionFrames = 0;
            UINT64 captureTime100ns = 0;
            error = audioCaptureClient->GetBuffer(&audioData, &numFramesToRead, &flags, &devicePositionFrames, &captureTime100ns);
            if (error.Error() == AUDCLNT_S_BUFFER_EMPTY) {
                return true;
            }
            if (FAILED(error.Error())) {
                return false;
            }
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                rtc::ExplicitZeroMemory(audioData, format.Format.nBlockAlign * numFramesToRead);
                RTC_DLOG(LS_WARNING) << "Captured audio is replaced by silence";
            } else {
                auto buffer = bytes::make_unique_binary(format.Format.nBlockAlign * numFramesToRead);
                memcpy(buffer.get(), audioData, format.Format.nBlockAlign * numFramesToRead);
                dataCallback(std::move(buffer), {});
            }
            error = audioCaptureClient->ReleaseBuffer(numFramesToRead);
            if (FAILED(error.Error())) {
                return false;
            }
            error = audioCaptureClient->GetNextPacketSize(&numFramesInNextPacket);
            if (FAILED(error.Error())) {
                return false;
            }
        }
        return true;
    }

    bool WinCoreDeviceModule::handleDataPlayback() {
        if (!running) {
            return false;
        }
        UINT32 numUnreadFrames = 0;
        _com_error error = audioClient->GetCurrentPadding(&numUnreadFrames);
        if (error.Error() == AUDCLNT_E_DEVICE_INVALIDATED) {
            RTC_DLOG(LS_ERROR) << "AUDCLNT_E_DEVICE_INVALIDATED";
            return false;
        }
        if (FAILED(error.Error())) {
            RTC_LOG(LS_ERROR) << "IAudioClient::GetCurrentPadding failed: " << core_audio_utility::ErrorToString(error);
            return false;
        }
        const UINT32 numRequestedFrames = endpointBufferSizeFrames - numUnreadFrames;
        if (numRequestedFrames == 0) {
            RTC_DLOG(LS_WARNING)
                << "Audio thread is signaled but no new audio samples are needed";
            return true;
        }
        uint8_t* audioData;
        error = audioRenderClient->GetBuffer(numRequestedFrames, &audioData);
        if (FAILED(error.Error())) {
            RTC_LOG(LS_ERROR) << "IAudioRenderClient::GetBuffer failed: " << core_audio_utility::ErrorToString(error);
            return false;
        }
        std::lock_guard queueLock(queueMutex);
        if (!queue.empty()) {
            memcpy(audioData, queue.front().get(), numRequestedFrames * format.Format.nBlockAlign);
            queue.pop();
        }
        error = audioRenderClient->ReleaseBuffer(numRequestedFrames, 0);
        if (FAILED(error.Error())) {
            RTC_LOG(LS_ERROR) << "IAudioRenderClient::ReleaseBuffer failed: " << core_audio_utility::ErrorToString(error);
        }
        return true;
    }

    void WinCoreDeviceModule::onData(bytes::unique_binary data) {
        std::lock_guard queueLock(queueMutex);
        queue.emplace(std::move(data));
    }

    void WinCoreDeviceModule::stop() {
        if (!running) return;
        running = false;
        if (FAILED(static_cast<_com_error>(audioClient->Stop()).Error())) {
            throw MediaDeviceError("Failed to stop audio client");
        }
        if (FAILED(static_cast<_com_error>(audioClient->Reset()).Error())) {
            throw MediaDeviceError("Failed to reset audio client");
        }
        if (!isCapture) {
            UINT32 num_queued_frames = 0;
            if (FAILED(audioClient->GetCurrentPadding(&num_queued_frames))) {
                throw MediaDeviceError("Failed to get current padding");
            }
            RTC_DCHECK_EQ(0u, num_queued_frames);
        }
        if (FAILED(static_cast<_com_error>(audioSessionControl->UnregisterAudioSessionNotification(this)).Error())) {
            throw MediaDeviceError("Failed to unregister audio session notification");
        }
        releaseCOMObjects();
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    void WinCoreDeviceModule::switchDevice() {
        if (core_audio_utility::NumberOfActiveDevices(isCapture ? eCapture:eRender) < 1) {
            throw MediaDeviceError("No active devices");
        }
        std::string newDeviceUID;
        switch (deviceIndex) {
        case 0:
            newDeviceUID = isCapture ? core_audio_utility::GetDefaultInputDeviceID() : core_audio_utility::GetDefaultOutputDeviceID();
            break;
        case 1:
            newDeviceUID = isCapture ? core_audio_utility::GetCommunicationsInputDeviceID() : core_audio_utility::GetCommunicationsOutputDeviceID();
            break;
        default:
            webrtc::AudioDeviceNames deviceNames;
            if (isCapture ? core_audio_utility::GetInputDeviceNames(&deviceNames) : core_audio_utility::GetOutputDeviceNames(&deviceNames)) {
                if (deviceIndex < deviceNames.size()) {
                    newDeviceUID = deviceNames[deviceIndex].unique_id;
                }
            }
        }
        if (newDeviceUID != deviceUID) {
            deviceUID = newDeviceUID;
            deviceIndex = 0;
        }
    }
} // ntgcalls

#endif