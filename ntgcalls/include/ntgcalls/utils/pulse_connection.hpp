//
// Created by Lauren on 22/09/24.
//

#pragma once

#ifdef IS_LINUX
#include <atomic>
#include <map>
#include <string>
#include <pulse/pulseaudio.h>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace ntgcalls::utils {

    class PulseConnection {
        pa_threaded_mainloop* pa_main_loop_;
        pa_mainloop_api* pa_main_loop_api_;
        pa_context* pa_context_;
        char pa_server_version_[32]{};
        std::atomic_bool version_received_, running_, pa_state_changed_ = false;
        pa_stream* stream_{};
        std::string device_id_;
        std::map<std::string, std::string> play_devices_, record_devices_;
        wrtc::utils::synchronized_callback<void(bytes::unique_binary)> data_callback_;
        bool is_capture_ = false;

        void pa_lock() const;

        void pa_unlock() const;

        void enable_read_callback();

        void disable_read_callback() const;

        void wait_for_operation_completion(pa_operation* pa_operation) const;

        static void pa_context_state_callback(pa_context* c, void* p_this);

        static void pa_server_info_callback(pa_context*, const pa_server_info* i, void* p_this);

        static void pa_stream_read_callback(pa_stream*, size_t, void* p_this);

        static void pa_stream_state_callback(pa_stream* p, void* p_this);

        static void pa_sink_info_callback(pa_context*, const pa_sink_info* i, int eol, void* p_this);

        static void pa_source_info_callback(pa_context*, const pa_source_info* i, int eol, void* p_this);

    public:
        PulseConnection();

        ~PulseConnection();

        std::string get_version();

        void disconnect();

        void setup_stream(const pa_sample_spec& sample_spec, std::string device_id, bool is_capture);

        void start(int64_t buffer_size);

        std::map<std::string, std::string> get_play_devices();

        std::map<std::string, std::string> get_record_devices();

        void on_data(const std::function<void(bytes::unique_binary)> &callback);

        void write_data(const bytes::unique_binary& data, size_t size) const;
    };

} // ntgcalls::utils

#endif