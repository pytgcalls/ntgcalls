//
// Created by Lauren on 20/09/24.
//

#ifdef IS_WINDOWS
#include <cmath>
#include <rtc_base/zero_memory.h>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/devices/win_core_device_module.hpp>

namespace ntgcalls::media::devices {

    WinCoreDeviceModule::WinCoreDeviceModule(const AudioDescription* desc, const bool is_capture, BaseSink* sink):
    BaseIO(sink),
    BaseDeviceModule(desc, is_capture),
    BaseReader(sink),
    AudioMixer(sink),
    com_initializer_(webrtc::ScopedCOMInitializer::kMTA),
    mmcss_registration_(L"Pro Audio") {
        RTC_DCHECK(com_initializer_.Succeeded());
        RTC_DCHECK(mmcss_registration_.Succeeded());

        audio_samples_event_.Set(CreateEvent(nullptr, false, false, nullptr));
        RTC_DCHECK(audio_samples_event_.IsValid());
        restart_event_.Set(CreateEvent(nullptr, false, false, nullptr));
        RTC_DCHECK(restart_event_.IsValid());
        stop_event_.Set(CreateEvent(nullptr, false, false, nullptr));
        RTC_DCHECK(stop_event_.IsValid());
        try {
            device_index_ = device_metadata_["index"];
            device_uid_ = device_metadata_["uid"];
            automatic_restart_ = device_metadata_["automatic_restart"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        if (device_index_ < 0) {
            throw MediaDeviceError("Invalid device index");
        }
    }

    WinCoreDeviceModule::~WinCoreDeviceModule() {
        SetEvent(stop_event_.Get());
        thread_.Finalize();
        ResetEvent(stop_event_.Get());
        ResetEvent(restart_event_.Get());
        ResetEvent(audio_samples_event_.Get());
        stop();
    }

    bool WinCoreDeviceModule::is_supported() {
        return core_audio_utility::IsMMCSSSupported();
    }

    std::vector<DeviceInfo> WinCoreDeviceModule::get_devices() {
        auto append_devices = [](const webrtc::AudioDeviceNames& device_names, std::vector<DeviceInfo>& devices, const bool is_microphone = false) {
            int index = 0;
            for (const auto& device : device_names) {
                json metadata = {
                    {"is_microphone", is_microphone},
                    {"index", index},
                    {"uid", device.unique_id},
                    {"automatic_restart", true}
                };
                devices.emplace_back(device.device_name, metadata.dump());
                index++;
            }
        };
        const webrtc::ScopedCOMInitializer com_initializer(webrtc::ScopedCOMInitializer::kMTA);
        webrtc::AudioDeviceNames device_names;
        std::vector<DeviceInfo> devices;
        core_audio_utility::GetInputDeviceNames(&device_names);
        append_devices(device_names, devices, true);
        device_names.clear();
        core_audio_utility::GetOutputDeviceNames(&device_names);
        append_devices(device_names, devices);
        return devices;
    }

    void WinCoreDeviceModule::open() {
        init();
        run_data_listener();
    }

    void WinCoreDeviceModule::init() {
        if (running_) return;
        running_ = true;
        const auto data_flow = is_capture_ ? eCapture : eRender;
        std::string device_id = webrtc::AudioDeviceName::kDefaultDeviceId;
        auto role = ERole();
        switch (device_index_) {
        case 0:
            role = eConsole;
            break;
        case 1:
            role = eCommunications;
            break;
        default:
            device_id = device_uid_;
            break;
        }

        const auto audio_client_version = core_audio_utility::GetAudioClientVersion();
        switch (audio_client_version) {
        case 3:
            RTC_LOG(LS_INFO) << "Using CoreAudioV3";
            audio_client_ = core_audio_utility::CreateClient3(device_id, data_flow, role);
            break;
        case 2:
            RTC_LOG(LS_INFO) << "Using CoreAudioV2";
            audio_client_ = core_audio_utility::CreateClient2(device_id, data_flow, role);
            break;
        default:
            RTC_LOG(LS_ERROR) << "Using CoreAudioV1";
            audio_client_ = core_audio_utility::CreateClient(device_id, data_flow, role);
        }
        if (!audio_client_) {
            throw MediaDeviceError("Failed to create audio client");
        }
        if (audio_client_version >= 2) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            if (FAILED(core_audio_utility::SetClientProperties(static_cast<IAudioClient2*>(audio_client_.Get())))) {
                throw MediaDeviceError("Failed to set client properties");
            }
        }

        webrtc::AudioParameters params;
        if (FAILED(core_audio_utility::GetPreferredAudioParameters(audio_client_.Get(), &params))) {
            throw MediaDeviceError("Failed to get preferred audio parameters");
        }

        WAVEFORMATEX* f = &format_.Format;
        f->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        f->nChannels = webrtc::dchecked_cast<WORD>(params.channels());
        f->nSamplesPerSec = webrtc::dchecked_cast<DWORD>(params.sample_rate());
        f->wBitsPerSample = webrtc::dchecked_cast<WORD>(params.bits_per_sample());
        f->nBlockAlign = f->wBitsPerSample / 8 * f->nChannels;
        f->nAvgBytesPerSec = f->nSamplesPerSec * f->nBlockAlign;
        f->cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        format_.Samples.wValidBitsPerSample = webrtc::dchecked_cast<WORD>(params.bits_per_sample());
        format_.dwChannelMask = f->nChannels == 1 ? KSAUDIO_SPEAKER_MONO : KSAUDIO_SPEAKER_STEREO;
        format_.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        rate_ = params.sample_rate();
        channels_ = params.channels();

        const auto audio_sink = dynamic_cast<AudioSink*>(sink_);
        if (auto config = audio_sink->get_config(); config->channel_count != channels_ || config->sample_rate != rate_) {
            RTC_LOG(LS_INFO) << "Updating Audio Configuration...";
            config->channel_count = channels_;
            config->sample_rate = rate_;
            dynamic_cast<AudioSink*>(sink_)->set_config(config);
        }

        // TODO: Low latency mode is not supported yet
        if (FAILED(core_audio_utility::SharedModeInitialize(audio_client_.Get(), &format_, audio_samples_event_, 0, true, &endpoint_buffer_size_frames_))) {
            throw MediaDeviceError("Failed to initialize shared mode");
        }

        if (!core_audio_utility::IsFormatSupported(audio_client_.Get(), AUDCLNT_SHAREMODE_SHARED, &format_)) {
            throw MediaDeviceError("Unsupported audio format with " + std::to_string(channels_) + " channels");
        }

        REFERENCE_TIME device_period;
        if (FAILED(core_audio_utility::GetDevicePeriod(audio_client_.Get(), AUDCLNT_SHAREMODE_SHARED, &device_period))) {
            throw MediaDeviceError("Failed to get device period");
        }

        const double device_period_in_seconds = static_cast<double>(core_audio_utility::ReferenceTimeToTimeDelta(device_period).ms()) / 1000.0;
        if (const int preferred_frames_per_buffer = static_cast<int>(lround(params.sample_rate() * device_period_in_seconds)); preferred_frames_per_buffer % params.frames_per_buffer()) {
            RTC_LOG(LS_WARNING) << "Preferred frames per buffer is not a multiple of frames per buffer";
        }

        audio_session_control_ = core_audio_utility::CreateAudioSessionControl(audio_client_.Get());
        if (!audio_session_control_.Get()) {
            throw MediaDeviceError("Failed to create audio session control");
        }

        AudioSessionState state;
        if (FAILED(audio_session_control_->GetState(&state))) {
            throw MediaDeviceError("Failed to get audio session state");
        }

        if (FAILED(audio_session_control_->RegisterAudioSessionNotification(this))) {
            throw MediaDeviceError("Failed to register audio session notification");
        }

        if (is_capture_) {
            audio_capture_client_ = core_audio_utility::CreateCaptureClient(audio_client_.Get());
        } else {
            audio_render_client_ = core_audio_utility::CreateRenderClient(audio_client_.Get());
            core_audio_utility::FillRenderEndpointBufferWithSilence(audio_client_.Get(), audio_render_client_.Get());
        }
        if (FAILED(static_cast<_com_error>(audio_client_->Start()).Error())) {
            throw MediaDeviceError("Failed to start audio client");
        }
    }

    void WinCoreDeviceModule::release_com_objects() {
        if (audio_render_client_.Get()) {
            audio_render_client_.Reset();
        }
        if (audio_capture_client_.Get()) {
            audio_capture_client_.Reset();
        }
        if (audio_render_client_.Get()) {
            audio_render_client_.Reset();
        }
        if (audio_client_) {
            audio_client_.Reset();
        }
        if (audio_session_control_.Get()) {
            audio_session_control_.Reset();
        }
    }

    HRESULT WinCoreDeviceModule::QueryInterface(const IID& riid, void** ppv_object) {
        if (ppv_object == nullptr) {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == __uuidof(IAudioSessionEvents)) {
            *ppv_object = static_cast<IAudioSessionEvents*>(this);
            return S_OK;
        }
        *ppv_object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WinCoreDeviceModule::AddRef() {
        return InterlockedIncrement(&ref_count_);
    }

    ULONG WinCoreDeviceModule::Release() {
        return InterlockedDecrement(&ref_count_);
    }

    HRESULT WinCoreDeviceModule::OnDisplayNameChanged(LPCWSTR new_display_name, LPCGUID event_context) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnIconPathChanged(LPCWSTR new_icon_path, LPCGUID event_context) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnSimpleVolumeChanged(float new_volume, BOOL new_mute, LPCGUID event_context) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnChannelVolumeChanged(DWORD channel_count, float new_channel_volume_array[], DWORD changed_channel, LPCGUID event_context) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnGroupingParamChanged(LPCGUID new_grouping_param, LPCGUID event_context) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnStateChanged(AudioSessionState new_state) {
        return S_OK;
    }

    HRESULT WinCoreDeviceModule::OnSessionDisconnected(const AudioSessionDisconnectReason disconnect_reason) {
        if (!automatic_restart_) {
            return S_OK;
        }
        if (is_restarting_) {
            return S_OK;
        }
        if (disconnect_reason == DisconnectReasonDeviceRemoval || disconnect_reason == DisconnectReasonFormatChanged) {
            is_restarting_ = true;
            SetEvent(restart_event_.Get());
        }
        return S_OK;
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    bool WinCoreDeviceModule::handle_restart_event() {
        bool restart_ok = true;
        try {
            stop();
            switch_device();
            init();
        } catch (...) {
            restart_ok = false;
        }
        is_restarting_ = false;
        return restart_ok;
    }

    void WinCoreDeviceModule::run_data_listener() {
        thread_ = webrtc::PlatformThread::SpawnJoinable(
            [this] {
                bool streaming = true;
                bool error = false;
                HANDLE wait_array[] = {stop_event_.Get(), restart_event_.Get(), audio_samples_event_.Get()};
                while (streaming && !error) {
                    switch (WaitForMultipleObjects(std::size(wait_array), wait_array, false, INFINITE)) {
                    case WAIT_OBJECT_0 + 0:
                        streaming = false;
                        break;
                    case WAIT_OBJECT_0 + 1:
                        error = !handle_restart_event();
                        break;
                    case WAIT_OBJECT_0 + 2:
                        if (is_capture_) {
                            error = !handle_data_record();
                        } else {
                            error = !handle_data_playback();
                        }
                        break;
                    default:
                        error = true;
                        break;
                    }
                }
                if (streaming && error) {
                    if (const _com_error result = audio_client_->Stop(); FAILED(result.Error())) {
                        RTC_LOG(LS_ERROR) << "IAudioClient::Stop failed: " << core_audio_utility::ErrorToString(result);
                    }
                }
            },
            "WinCoreAudio",
            webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime)
        );
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    bool WinCoreDeviceModule::handle_data_record() const {
        if (!running_) {
            return false;
        }
        UINT32 num_frames_in_next_packet = 0;
        _com_error error = audio_capture_client_->GetNextPacketSize(&num_frames_in_next_packet);
        if (error.Error() == AUDCLNT_E_DEVICE_INVALIDATED) {
            return true;
        }
        if (FAILED(error.Error())) {
            return false;
        }
        while (num_frames_in_next_packet > 0) {
            uint8_t* audio_data;
            UINT32 num_frames_to_read = 0;
            DWORD flags = 0;
            UINT64 device_position_frames = 0;
            UINT64 capture_time_100ns = 0;
            error = audio_capture_client_->GetBuffer(&audio_data, &num_frames_to_read, &flags, &device_position_frames, &capture_time_100ns);
            if (error.Error() == AUDCLNT_S_BUFFER_EMPTY) {
                return true;
            }
            if (FAILED(error.Error())) {
                return false;
            }
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                webrtc::ExplicitZeroMemory(audio_data, format_.Format.nBlockAlign * num_frames_to_read);
                RTC_DLOG(LS_WARNING) << "Captured audio is replaced by silence";
            } else {
                auto buffer = bytes::make_unique_binary(format_.Format.nBlockAlign * num_frames_to_read);
                std::memcpy(buffer.get(), audio_data, format_.Format.nBlockAlign * num_frames_to_read);
                data_callback_(std::move(buffer), {});
            }
            error = audio_capture_client_->ReleaseBuffer(num_frames_to_read);
            if (FAILED(error.Error())) {
                return false;
            }
            error = audio_capture_client_->GetNextPacketSize(&num_frames_in_next_packet);
            if (FAILED(error.Error())) {
                return false;
            }
        }
        return true;
    }

    bool WinCoreDeviceModule::handle_data_playback() {
        if (!running_) {
            return false;
        }
        UINT32 num_unread_frames = 0;
        _com_error error = audio_client_->GetCurrentPadding(&num_unread_frames);
        if (error.Error() == AUDCLNT_E_DEVICE_INVALIDATED) {
            RTC_DLOG(LS_ERROR) << "AUDCLNT_E_DEVICE_INVALIDATED";
            return false;
        }
        if (FAILED(error.Error())) {
            RTC_LOG(LS_ERROR) << "IAudioClient::GetCurrentPadding failed: " << core_audio_utility::ErrorToString(error);
            return false;
        }
        const UINT32 num_requested_frames = endpoint_buffer_size_frames_ - num_unread_frames;
        if (num_requested_frames == 0) {
            RTC_DLOG(LS_WARNING)
                << "Audio thread is signaled but no new audio samples are needed";
            return true;
        }
        uint8_t* audio_data;
        error = audio_render_client_->GetBuffer(num_requested_frames, &audio_data);
        if (FAILED(error.Error())) {
            RTC_LOG(LS_ERROR) << "IAudioRenderClient::GetBuffer failed: " << core_audio_utility::ErrorToString(error);
            return false;
        }
        const std::lock_guard queue_lock(queue_mutex_);
        if (!queue_.empty()) {
            std::memcpy(audio_data, queue_.front().get(), num_requested_frames * format_.Format.nBlockAlign);
            queue_.pop();
        }
        error = audio_render_client_->ReleaseBuffer(num_requested_frames, 0);
        if (FAILED(error.Error())) {
            RTC_LOG(LS_ERROR) << "IAudioRenderClient::ReleaseBuffer failed: " << core_audio_utility::ErrorToString(error);
        }
        return true;
    }

    void WinCoreDeviceModule::on_data(bytes::unique_binary data) {
        const std::lock_guard queue_lock(queue_mutex_);
        queue_.emplace(std::move(data));
    }

    void WinCoreDeviceModule::stop() {
        if (!running_) return;
        running_ = false;
        if (FAILED(static_cast<_com_error>(audio_client_->Stop()).Error())) {
            throw MediaDeviceError("Failed to stop audio client");
        }
        if (FAILED(static_cast<_com_error>(audio_client_->Reset()).Error())) {
            throw MediaDeviceError("Failed to reset audio client");
        }
        if (!is_capture_) {
            UINT32 num_queued_frames = 0;
            if (FAILED(audio_client_->GetCurrentPadding(&num_queued_frames))) {
                throw MediaDeviceError("Failed to get current padding");
            }
            RTC_DCHECK_EQ(0u, num_queued_frames);
        }
        if (FAILED(static_cast<_com_error>(audio_session_control_->UnregisterAudioSessionNotification(this)).Error())) {
            throw MediaDeviceError("Failed to unregister audio session notification");
        }
        release_com_objects();
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    void WinCoreDeviceModule::switch_device() {
        if (core_audio_utility::NumberOfActiveDevices(is_capture_ ? eCapture : eRender) < 1) {
            throw MediaDeviceError("No active devices");
        }
        std::string new_device_uid;
        switch (device_index_) {
        case 0:
            new_device_uid = is_capture_ ? core_audio_utility::GetDefaultInputDeviceID() : core_audio_utility::GetDefaultOutputDeviceID();
            break;
        case 1:
            new_device_uid = is_capture_ ? core_audio_utility::GetCommunicationsInputDeviceID() : core_audio_utility::GetCommunicationsOutputDeviceID();
            break;
        default:
            webrtc::AudioDeviceNames device_names;
            if (is_capture_ ? core_audio_utility::GetInputDeviceNames(&device_names) : core_audio_utility::GetOutputDeviceNames(&device_names)) {
                if (device_index_ < device_names.size()) {
                    new_device_uid = device_names[device_index_].unique_id;
                }
            }
        }
        if (new_device_uid != device_uid_) {
            device_uid_ = new_device_uid;
            device_index_ = 0;
        }
    }
} // ntgcalls::media::devices

#endif
