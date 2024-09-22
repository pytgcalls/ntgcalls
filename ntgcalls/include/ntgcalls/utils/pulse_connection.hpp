//
// Created by Laky64 on 22/09/24.
//

#pragma once
#include <atomic>
#include <map>


#ifdef IS_LINUX
#include <wrtc/utils/binary.hpp>
#include <pulse/pulseaudio.h>
#include <string>

namespace ntgcalls {

    class PulseConnection {
        pa_threaded_mainloop* paMainloop;
        pa_mainloop_api* paMainloopApi;
        pa_context* paContext;
        char paServerVersion[32]{};
        std::atomic_bool versionReceived, recording, paStateChanged = false;
        pa_stream* stream{};
        bytes::unique_binary buffer;
        std::string deviceID;
        std::map<std::string, std::string> playDevices, recordDevices;

        void paLock() const;

        void paUnLock() const;

        void enableReadCallback();

        void disableReadCallback() const;

        void waitForOperationCompletion(pa_operation* paOperation) const;

        static void paContextStateCallback(pa_context* c, void* pThis);

        static void paServerInfoCallback(pa_context*, const pa_server_info* i, void* pThis);

        static void paStreamReadCallback(pa_stream*, size_t, void* pThis);

        static void paStreamStateCallback(pa_stream* p, void* pThis);

        static void paSinkInfoCallback(pa_context*, const pa_sink_info* i, int eol, void* pThis);

        static void paSourceInfoCallback(pa_context*, const pa_source_info* i, int eol, void* pThis);

    public:
        PulseConnection();

        std::string getVersion();

        void disconnect();

        void setupStream(const pa_sample_spec& sampleSpec, std::string deviceId, bool isCapture);

        bytes::unique_binary read(int64_t size);

        std::map<std::string, std::string> getPlayDevices();

        std::map<std::string, std::string> getRecordDevices();
    };

} // ntgcalls

#endif