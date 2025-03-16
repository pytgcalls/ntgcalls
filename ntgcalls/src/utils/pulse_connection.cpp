//
// Created by Laky64 on 22/09/24.
//

#include <ntgcalls/utils/pulse_connection.hpp>

#ifdef IS_LINUX
#include <ntgcalls/exceptions.hpp>
#include <modules/audio_device/linux/audio_device_pulse_linux.h>

#define LATE(sym) \
LATESYM_GET(webrtc::adm_linux_pulse::PulseAudioSymbolTable, GetPulseSymbolTable(), sym)

namespace ntgcalls {
    PulseConnection::PulseConnection() {
        paMainloop = LATE(pa_threaded_mainloop_new)();
        if (!paMainloop) {
            throw MediaDeviceError("Cannot create mainloop");
        }
        if (const auto err = LATE(pa_threaded_mainloop_start)(paMainloop); err != PA_OK) {
            throw MediaDeviceError("Cannot start mainloop, error=" + std::to_string(err));
        }
        paLock();
        paMainloopApi = LATE(pa_threaded_mainloop_get_api)(paMainloop);
        if (!paMainloopApi) {
            paUnLock();
            throw MediaDeviceError("Cannot get mainloop api");
        }
        paContext = LATE(pa_context_new)(paMainloopApi, "NTgCalls VoiceEngine");
        if (!paContext) {
            paUnLock();
            throw MediaDeviceError("Cannot create context");
        }
        LATE(pa_context_set_state_callback)(paContext, paContextStateCallback, this);
        paStateChanged = false;

        if (const auto err = LATE(pa_context_connect)(paContext, nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr); err != PA_OK) {
            paUnLock();
            throw MediaDeviceError("Cannot connect to pulseaudio, error=" + std::to_string(err));
        }

        while (!paStateChanged) {
            LATE(pa_threaded_mainloop_wait)(paMainloop);
        }

        if (const auto state = LATE(pa_context_get_state)(paContext); state != PA_CONTEXT_READY) {
            std::string error;
            if (state == PA_CONTEXT_FAILED) {
                error = "Failed to connect to PulseAudio sound server";
            } else if (state == PA_CONTEXT_TERMINATED) {
                error = "PulseAudio connection terminated early";
            } else {
                error = "Unknown problem connecting to PulseAudio";
            }
            paUnLock();
            throw MediaDeviceError(error);
        }
        paUnLock();
    }

    void PulseConnection::paLock() const {
        LATE(pa_threaded_mainloop_lock)(paMainloop);
    }

    void PulseConnection::paUnLock() const {
        LATE(pa_threaded_mainloop_unlock)(paMainloop);
    }

    std::string PulseConnection::getVersion() {
        if (versionReceived) {
            return paServerVersion;
        }
        paLock();
        pa_operation* paOperation = nullptr;
        paOperation = LATE(pa_context_get_server_info)(paContext, paServerInfoCallback, this);
        waitForOperationCompletion(paOperation);
        paUnLock();
        versionReceived = true;
        return paServerVersion;
    }

    void PulseConnection::enableReadCallback() {
        LATE(pa_stream_set_read_callback)(stream, &paStreamReadCallback, this);
    }

    void PulseConnection::disableReadCallback() const {
        LATE(pa_stream_set_read_callback)(stream, nullptr, nullptr);
    }

    void PulseConnection::waitForOperationCompletion(pa_operation* paOperation) const {
        if (!paOperation) {
            RTC_LOG(LS_ERROR) << "PaOperation NULL in WaitForOperationCompletion";
            return;
        }
        while (LATE(pa_operation_get_state)(paOperation) == PA_OPERATION_RUNNING) {
            LATE(pa_threaded_mainloop_wait)(paMainloop);
        }
        LATE(pa_operation_unref)(paOperation);
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void PulseConnection::paContextStateCallback(pa_context* c, void* pThis) {
        const auto thiz = static_cast<PulseConnection*>(pThis);
        switch (LATE(pa_context_get_state)(c)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_READY:
            thiz->paStateChanged = true;
            LATE(pa_threaded_mainloop_signal)(thiz->paMainloop, 0);
            break;
        }
    }

    void PulseConnection::paServerInfoCallback(pa_context*, const pa_server_info* i, void* pThis) {
        const auto thiz = static_cast<PulseConnection*>(pThis);
        strncpy(thiz->paServerVersion, i->server_version, 31);
        thiz->paServerVersion[31] = '\0';
        LATE(pa_threaded_mainloop_signal)(thiz->paMainloop, 0);
    }

    void PulseConnection::paSinkInfoCallback(pa_context*, const pa_sink_info* i, const int eol, void* pThis) {
        const auto thiz = static_cast<PulseConnection*>(pThis);
        if (eol) {
            LATE(pa_threaded_mainloop_signal)(thiz->paMainloop, 0);
            return;
        }
        thiz->playDevices[i->name] = i->description;
    }

    void PulseConnection::paSourceInfoCallback(pa_context*, const pa_source_info* i, const int eol, void* pThis) {
        const auto thiz = static_cast<PulseConnection*>(pThis);
        if (eol) {
            LATE(pa_threaded_mainloop_signal)(thiz->paMainloop, 0);
            return;
        }
        thiz->recordDevices[i->name] = i->description;
    }

    std::map<std::string, std::string> PulseConnection::getPlayDevices() {
        paLock();
        pa_operation* paOperation = nullptr;
        playDevices.clear();
        paOperation = LATE(pa_context_get_sink_info_list)(paContext, paSinkInfoCallback, this);
        waitForOperationCompletion(paOperation);
        paUnLock();
        return playDevices;
    }

    std::map<std::string, std::string> PulseConnection::getRecordDevices() {
        paLock();
        pa_operation* paOperation = nullptr;
        recordDevices.clear();
        paOperation = LATE(pa_context_get_source_info_list)(paContext, paSourceInfoCallback, this);
        waitForOperationCompletion(paOperation);
        paUnLock();
        return recordDevices;
    }

    void PulseConnection::onData(const std::function<void(bytes::unique_binary)>& callback) {
        dataCallback = callback;
    }

    void PulseConnection::writeData(const bytes::unique_binary& data, const size_t size) const {
        if (!running) return;
        paLock();
        if (LATE(pa_stream_write)(stream, data.get(), size, nullptr, static_cast<int64_t>(0), PA_SEEK_RELATIVE) != PA_OK) {
            throw MediaDeviceError("Failed to write data to stream, err=" + std::to_string(LATE(pa_context_errno)(paContext)));
        }
        paUnLock();
    }

    void PulseConnection::paStreamStateCallback(pa_stream*, void* pThis) {
        LATE(pa_threaded_mainloop_signal)(static_cast<PulseConnection*>(pThis)->paMainloop, 0);
    }

    void PulseConnection::paStreamReadCallback(pa_stream*, const size_t size, void* pThis) {
        const auto thiz = static_cast<PulseConnection*>(pThis);
        size_t nBytes = size;
        while(nBytes > 0) {
            size_t count = nBytes;
            const void *audio_data;
            const int result = LATE(pa_stream_peek)(thiz->stream, &audio_data, &count);
            if(count == 0) {
                return;
            }
            if(audio_data == nullptr) {
                LATE(pa_stream_drop)(thiz->stream);
                return;
            }
            auto buffer = bytes::make_unique_binary(nBytes);
            memcpy(buffer.get(), audio_data, count);
            thiz->dataCallback(std::move(buffer));
            if(result != 0) {
                return;
            }
            LATE(pa_stream_drop)(thiz->stream);
            nBytes -= count;
        }
    }

    void PulseConnection::disconnect() {
        paLock();
        if (running) {
            if (isCapture) disableReadCallback();
            running = false;
        }
        if (stream) {
            if (LATE(pa_stream_get_state)(stream) != PA_STREAM_UNCONNECTED) {
                if (LATE(pa_stream_disconnect)(stream) != PA_OK) {
                    paUnLock();
                    throw MediaDeviceError("Failed to disconnect stream, err=" + std::to_string(LATE(pa_context_errno)(paContext)));
                }
                RTC_LOG(LS_VERBOSE) << "Disconnected recording";
            }
            LATE(pa_stream_unref)(stream);
        }
        LATE(pa_context_disconnect)(paContext);
        LATE(pa_context_unref)(paContext);
        paUnLock();
        LATE(pa_threaded_mainloop_stop)(paMainloop);
        LATE(pa_threaded_mainloop_free)(paMainloop);
    }

    void PulseConnection::setupStream(const pa_sample_spec& sampleSpec, std::string deviceId, const bool isCapture) {
        stream = LATE(pa_stream_new)(paContext, isCapture ? "recStream":"playStream", &sampleSpec, nullptr);
        if (!stream) {
            throw MediaDeviceError("Cannot create stream, err=" + std::to_string(LATE(pa_context_errno)(paContext)));
        }
        this->deviceID = std::move(deviceId);
        this->isCapture = isCapture;
        LATE(pa_stream_set_state_callback)(stream, paStreamStateCallback, this);
    }

    void PulseConnection::start(const int64_t bufferSize) {
        if (deviceID.empty()) {
            throw MediaDeviceError("No device selected");
        }
        paLock();
        pa_buffer_attr buffer_attr;
        buffer_attr.maxlength = -1;
        buffer_attr.tlength = -1;
        buffer_attr.prebuf = -1;
        buffer_attr.minreq = -1;
        buffer_attr.fragsize = bufferSize;
        if (isCapture) {
            if (LATE(pa_stream_connect_record)(stream, deviceID.c_str(), &buffer_attr, PA_STREAM_NOFLAGS) != PA_OK) {
                throw MediaDeviceError("cannot connect to stream");
            }
        } else {
            if (LATE(pa_stream_connect_playback)(stream, deviceID.c_str(), &buffer_attr, PA_STREAM_NOFLAGS, nullptr, nullptr) != PA_OK) {
                throw MediaDeviceError("cannot connect to stream");
            }
        }

        RTC_LOG(LS_VERBOSE) << "Connecting stream";
        while (LATE(pa_stream_get_state)(stream) != PA_STREAM_READY) {
            LATE(pa_threaded_mainloop_wait)(paMainloop);
        }
        RTC_LOG(LS_VERBOSE) << "Connected stream";
        if (isCapture) {
            enableReadCallback();
        }
        paUnLock();
        running = true;
    }
} // ntgcalls

#endif