//
// Created by Laky64 on 20/09/2024.
//

#pragma once
#include <ntgcalls/io/base_reader.hpp>

#ifdef IS_WINDOWS
#include <ntgcalls/devices/base_device_module.hpp>
#include <modules/audio_device/win/core_audio_utility_win.h>
#include <rtc_base/win/scoped_com_initializer.h>
#include <thread>
#include <ntgcalls/devices/device_info.hpp>

namespace ntgcalls {
    using Microsoft::WRL::ComPtr;
    using namespace webrtc::webrtc_win;

    class WinCoreDeviceModule final: public BaseDeviceModule, public BaseReader, public IAudioSessionEvents {
        ScopedHandle audioSamplesEvent, restartEvent, stopEvent;
        WAVEFORMATEXTENSIBLE format = {};
        uint32_t endpointBufferSizeFrames = 0;
        ComPtr<IAudioClient> audioClient;
        ComPtr<IAudioSessionControl> audioSessionControl;
        LONG refCount = 1;
        std::atomic_bool isRestarting, isInitialized = false;
        webrtc::ScopedCOMInitializer comInitializer;
        ScopedMMCSSRegistration mmcssRegistration;
        std::thread thread;
        bool automaticRestart = false;
        int deviceIndex = -1;
        std::string deviceUID;
        ComPtr<IAudioCaptureClient> audioCaptureClient;
        ComPtr<IAudioRenderClient> audioRenderClient;

        void init();

        void releaseCOMObjects();

        bool handleRestartEvent();

        void runDataListener();

        void stop();

        void switchDevice();

        bool handleDataEvent() const;

        HRESULT QueryInterface(const IID& riid, void** ppvObject) override;

        ULONG AddRef() override;

        ULONG Release() override;

        HRESULT OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext) override;

        HRESULT OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext) override;

        HRESULT OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext) override;

        HRESULT OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[], DWORD ChangedChannel, LPCGUID EventContext) override;

        HRESULT OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext) override;

        HRESULT OnStateChanged(AudioSessionState NewState) override;

        HRESULT OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason) override;

    public:
        explicit WinCoreDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink *sink);

        ~WinCoreDeviceModule() override;

        static bool isSupported();

        static std::vector<DeviceInfo> getDevices();

        void open() override;
    };

} // ntgcalls

#endif