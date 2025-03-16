//
// Created by Laky64 on 22/09/24.
//

#pragma once
#include <atomic>
#include <map>
#include <wrtc/utils/syncronized_callback.hpp>


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
        std::atomic_bool versionReceived, running, paStateChanged = false;
        pa_stream* stream{};
        std::string deviceID;
        std::map<std::string, std::string> playDevices, recordDevices;
        wrtc::synchronized_callback<bytes::unique_binary> dataCallback;
        bool isCapture = false;

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

        void start(int64_t bufferSize);

        std::map<std::string, std::string> getPlayDevices();

        std::map<std::string, std::string> getRecordDevices();

        void onData(const std::function<void(bytes::unique_binary)> &callback);

        void writeData(const bytes::unique_binary& data, size_t size) const;
    };

} // ntgcalls

#endif