//
// Log bridge: routes the core LogSink messages to the JS console. Messages come
// from C++ worker threads, so they cross to the JS thread through a
// ThreadSafeFunction. Logging is silent by default (min level above Error); the
// addon exposes setLogLevel() so a consumer can opt in to Error/Warning/Info/Debug.
//
#pragma once

#include <atomic>
#include <string>

#include <napi.h>

#include <ntgcalls/utils/log_sink_impl.hpp>

namespace ntgcalls::support {
    inline std::atomic<int> min_log_level{16};

    inline void set_log_level(const int level) {
        min_log_level.store(level);
    }

    inline const char* log_console_method(const utils::LogSink::Level level) {
        switch (level) {
            case utils::LogSink::Level::Error:
                return "error";
            case utils::LogSink::Level::Warning:
                return "warn";
            case utils::LogSink::Level::Info:
                return "info";
            default:
                return "debug";
        }
    }

    inline void install_log_bridge(const Napi::Env env) {
        const auto console = env.Global().Get("console").As<Napi::Object>();
        auto tsfn = Napi::ThreadSafeFunction::New(env, console.Get("log").As<Napi::Function>(), "ntg_log", 0, 1);
        tsfn.Unref(env);
        utils::LogSink::register_logger([tsfn](const utils::LogSink::LogMessage& message) mutable {
            if (static_cast<int>(message.level) < min_log_level.load()) {
                return;
            }
            const auto payload = new utils::LogSink::LogMessage(message);
            tsfn.NonBlockingCall(payload, [](const Napi::Env cbEnv, Napi::Function, utils::LogSink::LogMessage* data) {
                const auto console = cbEnv.Global().Get("console").As<Napi::Object>();
                const auto name = data->source == utils::LogSink::Source::Self ? "ntgcalls" : "webrtc";
                const auto text = "[" + std::string(name) + "] " + data->file + ":" + std::to_string(data->line) + " " + data->message;
                console.Get(log_console_method(data->level)).As<Napi::Function>().Call(console, {Napi::String::New(cbEnv, text)});
                delete data;
            });
        });
    }
} // ntgcalls::support
