//
// Created by Laky64 on 18/09/24.
//

#pragma once
#include <atomic>

#ifdef IS_LINUX
#include <ntgcalls/devices/base_device_module.hpp>
#include <pulse/pulseaudio.h>

namespace ntgcalls {

    class PulseDeviceModule final: public BaseDeviceModule {
        pa_threaded_mainloop* paMainloop;
        pa_mainloop_api* paMainloopApi;
        pa_context* paContext;
        bool paStateChanged;
        char paServerVersion[32]{};
        pa_stream* stream{};
        bool recording = false;
        bytes::unique_binary recBuffer;

        void paLock() const;

        void paUnLock() const;

        void enableReadCallback();

        void disableReadCallback() const;

        void waitForOperationCompletion(pa_operation* paOperation) const;

        void checkPulseAudioVersion();

        static void paContextStateCallback(pa_context* c, void* pThis);

        static void paServerInfoCallback(pa_context*, const pa_server_info* i, void* pThis);

        static void paStreamReadCallback(pa_stream*, size_t, void* pThis);

        static void paStreamStateCallback(pa_stream* p, void* pThis);

    public:
        PulseDeviceModule(const AudioDescription* desc, bool isCapture);

        [[nodiscard]] bytes::unique_binary read(int64_t size) override;

        static bool isSupported();

        void close() override;
    };

} // pulse

#endif