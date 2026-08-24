//
// Created by Lauren on 22/09/24.
//

#ifdef IS_LINUX
#include <modules/audio_device/linux/audio_device_pulse_linux.h>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/utils/pulse_connection.hpp>

#define LATE(sym) \
    LATESYM_GET(webrtc::adm_linux_pulse::PulseAudioSymbolTable, GetPulseSymbolTable(), sym)

namespace ntgcalls::utils {
    PulseConnection::PulseConnection() {
        pa_main_loop_ = LATE(pa_threaded_mainloop_new)();
        if (!pa_main_loop_) {
            throw MediaDeviceError("Cannot create mainloop");
        }
        if (const auto err = LATE(pa_threaded_mainloop_start)(pa_main_loop_); err != PA_OK) {
            throw MediaDeviceError("Cannot start mainloop, error=" + std::to_string(err));
        }
        pa_lock();
        pa_main_loop_api_ = LATE(pa_threaded_mainloop_get_api)(pa_main_loop_);
        if (!pa_main_loop_api_) {
            pa_unlock();
            throw MediaDeviceError("Cannot get mainloop api");
        }
        pa_context_ = LATE(pa_context_new)(pa_main_loop_api_, "NTgCalls VoiceEngine");
        if (!pa_context_) {
            pa_unlock();
            throw MediaDeviceError("Cannot create context");
        }
        LATE(pa_context_set_state_callback)(pa_context_, pa_context_state_callback, this);
        pa_state_changed_ = false;

        if (const auto err = LATE(pa_context_connect)(pa_context_, nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr); err != PA_OK) {
            pa_unlock();
            throw MediaDeviceError("Cannot connect to pulseaudio, error=" + std::to_string(err));
        }

        while (!pa_state_changed_) {
            LATE(pa_threaded_mainloop_wait)(pa_main_loop_);
        }

        if (const auto state = LATE(pa_context_get_state)(pa_context_); state != PA_CONTEXT_READY) {
            std::string error;
            if (state == PA_CONTEXT_FAILED) {
                error = "Failed to connect to PulseAudio sound server";
            } else if (state == PA_CONTEXT_TERMINATED) {
                error = "PulseAudio connection terminated early";
            } else {
                error = "Unknown problem connecting to PulseAudio";
            }
            pa_unlock();
            throw MediaDeviceError(error);
        }
        pa_unlock();
    }

    PulseConnection::~PulseConnection() {
        disconnect();
    }

    void PulseConnection::pa_lock() const {
        LATE(pa_threaded_mainloop_lock)(pa_main_loop_);
    }

    void PulseConnection::pa_unlock() const {
        LATE(pa_threaded_mainloop_unlock)(pa_main_loop_);
    }

    std::string PulseConnection::get_version() {
        if (version_received_) {
            return pa_server_version_;
        }
        pa_lock();
        pa_operation* pa_operation = nullptr;
        pa_operation = LATE(pa_context_get_server_info)(pa_context_, pa_server_info_callback, this);
        wait_for_operation_completion(pa_operation);
        pa_unlock();
        version_received_ = true;
        return pa_server_version_;
    }

    void PulseConnection::enable_read_callback() {
        LATE(pa_stream_set_read_callback)(stream_, &pa_stream_read_callback, this);
    }

    void PulseConnection::disable_read_callback() const {
        LATE(pa_stream_set_read_callback)(stream_, nullptr, nullptr);
    }

    void PulseConnection::wait_for_operation_completion(pa_operation* pa_operation) const {
        if (!pa_operation) {
            RTC_LOG(LS_ERROR) << "PaOperation NULL in WaitForOperationCompletion";
            return;
        }
        while (LATE(pa_operation_get_state)(pa_operation) == PA_OPERATION_RUNNING) {
            LATE(pa_threaded_mainloop_wait)(pa_main_loop_);
        }
        LATE(pa_operation_unref)(pa_operation);
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void PulseConnection::pa_context_state_callback(pa_context* c, void* p_this) {
        const auto thiz = static_cast<PulseConnection*>(p_this);
        switch (LATE(pa_context_get_state)(c)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_READY:
            thiz->pa_state_changed_ = true;
            LATE(pa_threaded_mainloop_signal)(thiz->pa_main_loop_, 0);
            break;
        }
    }

    void PulseConnection::pa_server_info_callback(pa_context*, const pa_server_info* i, void* p_this) {
        const auto thiz = static_cast<PulseConnection*>(p_this);
        strncpy(thiz->pa_server_version_, i->server_version, 31);
        thiz->pa_server_version_[31] = '\0';
        LATE(pa_threaded_mainloop_signal)(thiz->pa_main_loop_, 0);
    }

    void PulseConnection::pa_sink_info_callback(pa_context*, const pa_sink_info* i, const int eol, void* p_this) {
        const auto thiz = static_cast<PulseConnection*>(p_this);
        if (eol) {
            LATE(pa_threaded_mainloop_signal)(thiz->pa_main_loop_, 0);
            return;
        }
        thiz->play_devices_[i->name] = i->description;
    }

    void PulseConnection::pa_source_info_callback(pa_context*, const pa_source_info* i, const int eol, void* p_this) {
        const auto thiz = static_cast<PulseConnection*>(p_this);
        if (eol) {
            LATE(pa_threaded_mainloop_signal)(thiz->pa_main_loop_, 0);
            return;
        }
        thiz->record_devices_[i->name] = i->description;
    }

    std::map<std::string, std::string> PulseConnection::get_play_devices() {
        pa_lock();
        pa_operation* pa_operation = nullptr;
        play_devices_.clear();
        pa_operation = LATE(pa_context_get_sink_info_list)(pa_context_, pa_sink_info_callback, this);
        wait_for_operation_completion(pa_operation);
        pa_unlock();
        return play_devices_;
    }

    std::map<std::string, std::string> PulseConnection::get_record_devices() {
        pa_lock();
        pa_operation* pa_operation = nullptr;
        record_devices_.clear();
        pa_operation = LATE(pa_context_get_source_info_list)(pa_context_, pa_source_info_callback, this);
        wait_for_operation_completion(pa_operation);
        pa_unlock();
        return record_devices_;
    }

    void PulseConnection::on_data(const std::function<void(bytes::unique_binary)>& callback) {
        data_callback_ = callback;
    }

    void PulseConnection::write_data(const bytes::unique_binary& data, const size_t size) const {
        if (!running_) return;
        pa_lock();
        if (LATE(pa_stream_write)(stream_, data.get(), size, nullptr, static_cast<int64_t>(0), PA_SEEK_RELATIVE) != PA_OK) {
            throw MediaDeviceError("Failed to write data to stream, err=" + std::to_string(LATE(pa_context_errno)(pa_context_)));
        }
        pa_unlock();
    }

    void PulseConnection::pa_stream_state_callback(pa_stream*, void* p_this) {
        LATE(pa_threaded_mainloop_signal)(static_cast<PulseConnection*>(p_this)->pa_main_loop_, 0);
    }

    void PulseConnection::pa_stream_read_callback(pa_stream*, const size_t size, void* p_this) {
        const auto thiz = static_cast<PulseConnection*>(p_this);
        size_t n_bytes = size;
        while (n_bytes > 0) {
            size_t count = n_bytes;
            const void* audio_data;
            const int result = LATE(pa_stream_peek)(thiz->stream_, &audio_data, &count);
            if (count == 0) {
                return;
            }
            if (audio_data == nullptr) {
                LATE(pa_stream_drop)(thiz->stream_);
                return;
            }
            auto buffer = bytes::make_unique_binary(n_bytes);
            std::memcpy(buffer.get(), audio_data, count);
            thiz->data_callback_(std::move(buffer));
            if (result != 0) {
                return;
            }
            LATE(pa_stream_drop)(thiz->stream_);
            n_bytes -= count;
        }
    }

    void PulseConnection::disconnect() {
        if (!pa_main_loop_) {
            return;
        }
        pa_lock();
        if (running_) {
            if (is_capture_) disable_read_callback();
            running_ = false;
        }
        if (stream_) {
            if (LATE(pa_stream_get_state)(stream_) != PA_STREAM_UNCONNECTED) {
                if (LATE(pa_stream_disconnect)(stream_) != PA_OK) {
                    pa_unlock();
                    throw MediaDeviceError("Failed to disconnect stream, err=" + std::to_string(LATE(pa_context_errno)(pa_context_)));
                }
                RTC_LOG(LS_VERBOSE) << "Disconnected recording";
            }
            LATE(pa_stream_unref)(stream_);
            stream_ = nullptr;
        }
        LATE(pa_context_disconnect)(pa_context_);
        LATE(pa_context_unref)(pa_context_);
        pa_context_ = nullptr;
        pa_unlock();
        LATE(pa_threaded_mainloop_stop)(pa_main_loop_);
        LATE(pa_threaded_mainloop_free)(pa_main_loop_);
        pa_main_loop_ = nullptr;
        data_callback_ = nullptr;
    }

    void PulseConnection::setup_stream(const pa_sample_spec& sample_spec, std::string device_id, const bool is_capture) {
        stream_ = LATE(pa_stream_new)(pa_context_, is_capture ? "recStream" : "playStream", &sample_spec, nullptr);
        if (!stream_) {
            throw MediaDeviceError("Cannot create stream, err=" + std::to_string(LATE(pa_context_errno)(pa_context_)));
        }
        this->device_id_ = std::move(device_id);
        this->is_capture_ = is_capture;
        LATE(pa_stream_set_state_callback)(stream_, pa_stream_state_callback, this);
    }

    void PulseConnection::start(const int64_t buffer_size) {
        if (device_id_.empty()) {
            throw MediaDeviceError("No device selected");
        }
        pa_lock();
        pa_buffer_attr buffer_attr;
        buffer_attr.maxlength = -1;
        buffer_attr.tlength = -1;
        buffer_attr.prebuf = -1;
        buffer_attr.minreq = -1;
        buffer_attr.fragsize = buffer_size;
        if (is_capture_) {
            if (LATE(pa_stream_connect_record)(stream_, device_id_.c_str(), &buffer_attr, PA_STREAM_NOFLAGS) != PA_OK) {
                throw MediaDeviceError("cannot connect to stream");
            }
        } else {
            if (LATE(pa_stream_connect_playback)(stream_, device_id_.c_str(), &buffer_attr, PA_STREAM_NOFLAGS, nullptr, nullptr) != PA_OK) {
                throw MediaDeviceError("cannot connect to stream");
            }
        }

        RTC_LOG(LS_VERBOSE) << "Connecting stream";
        while (LATE(pa_stream_get_state)(stream_) != PA_STREAM_READY) {
            LATE(pa_threaded_mainloop_wait)(pa_main_loop_);
        }
        RTC_LOG(LS_VERBOSE) << "Connected stream";
        if (is_capture_) {
            enable_read_callback();
        }
        pa_unlock();
        running_ = true;
    }
} // ntgcalls::utils

#endif
