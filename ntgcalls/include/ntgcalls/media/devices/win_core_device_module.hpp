//
// Created by Lauren on 20/09/24.
//

#pragma once

#ifdef IS_WINDOWS
#include <queue>
#include <modules/audio_device/win/core_audio_utility_win.h>
#include <ntgcalls/io/audio_mixer.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/devices/base_device_module.hpp>
#include <ntgcalls/media/devices/device_info.hpp>
#include <rtc_base/platform_thread.h>
#include <rtc_base/win/scoped_com_initializer.h>

namespace ntgcalls::media::devices {
    using Microsoft::WRL::ComPtr;
    using namespace webrtc::webrtc_win;

    class WinCoreDeviceModule final: public BaseDeviceModule, public io::BaseReader, public io::AudioMixer, public IAudioSessionEvents {
        ScopedHandle audio_samples_event_, restart_event_, stop_event_;
        WAVEFORMATEXTENSIBLE format_ = {};
        uint32_t endpoint_buffer_size_frames_ = 0;
        ComPtr<IAudioClient> audio_client_;
        ComPtr<IAudioSessionControl> audio_session_control_;
        LONG ref_count_ = 1;
        std::atomic_bool is_restarting_ = false;
        webrtc::ScopedCOMInitializer com_initializer_;
        ScopedMMCSSRegistration mmcss_registration_;
        webrtc::PlatformThread thread_;
        bool automatic_restart_ = false;
        int device_index_ = -1;
        std::string device_uid_;
        ComPtr<IAudioCaptureClient> audio_capture_client_;
        ComPtr<IAudioRenderClient> audio_render_client_;
        std::mutex queue_mutex_;
        std::queue<bytes::unique_binary> queue_;

        void init();

        void release_com_objects();

        bool handle_restart_event();

        void run_data_listener();

        void stop();

        void switch_device();

        bool handle_data_record() const;

        bool handle_data_playback();

        HRESULT QueryInterface(const IID& riid, void** ppv_object) override;

        ULONG AddRef() override;

        ULONG Release() override;

        HRESULT OnDisplayNameChanged(LPCWSTR new_display_name, LPCGUID event_context) override;

        HRESULT OnIconPathChanged(LPCWSTR new_icon_path, LPCGUID event_context) override;

        HRESULT OnSimpleVolumeChanged(float new_volume, BOOL new_mute, LPCGUID event_context) override;

        HRESULT OnChannelVolumeChanged(DWORD channel_count, float new_channel_volume_array[], DWORD changed_channel, LPCGUID event_context) override;

        HRESULT OnGroupingParamChanged(LPCGUID new_grouping_param, LPCGUID event_context) override;

        HRESULT OnStateChanged(AudioSessionState new_state) override;

        HRESULT OnSessionDisconnected(AudioSessionDisconnectReason disconnect_reason) override;

    protected:
        void on_data(bytes::unique_binary data) override;

    public:
        explicit WinCoreDeviceModule(const AudioDescription* desc, bool is_capture, BaseSink* sink);

        ~WinCoreDeviceModule() override;

        static bool is_supported();

        static std::vector<DeviceInfo> get_devices();

        void open() override;
    };

} // ntgcalls::media::devices

#endif
