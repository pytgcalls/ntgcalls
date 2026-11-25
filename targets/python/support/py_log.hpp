//
// Python binding support -- generated bindings only, not part of the core lib.
//
// The log bridge takes the GIL from the LogSink thread. A thread that asks for
// the GIL once the interpreter is finalizing hangs forever, and the LogSink
// static destructor then waits for it at exit, so an atexit hook closes the
// bridge and drains the messages in flight while the interpreter is still up.
//
#pragma once
#include <condition_variable>
#include <mutex>
#include <string>
#include <ntgcalls/utils/log_sink_impl.hpp>
#include <pybind11/pybind11.h>

#if PY_VERSION_HEX < 0x030D0000
#define Py_IsFinalizing _Py_IsFinalizing
#endif

namespace py = pybind11;

namespace ntgcalls::support {
    class LogBridge {
        bool closed_ = false;
        int in_flight_ = 0;
        std::mutex mutex_;
        std::condition_variable idle_;

    public:
        class Guard {
            LogBridge& bridge_;

        public:
            explicit Guard(LogBridge& bridge): bridge_(bridge) {}

            ~Guard() {
                bridge_.leave();
            }
        };

        static LogBridge& instance() {
            static auto* bridge = new LogBridge();
            return *bridge;
        }

        bool enter() {
            const std::lock_guard lock(mutex_);
            if (closed_) {
                return false;
            }
            ++in_flight_;
            return true;
        }

        void leave() {
            {
                const std::lock_guard lock(mutex_);
                --in_flight_;
            }
            idle_.notify_all();
        }

        static void close() {
            auto& bridge = instance();
            {
                const std::lock_guard lock(bridge.mutex_);
                bridge.closed_ = true;
            }
            const py::gil_scoped_release release;
            std::unique_lock lock(bridge.mutex_);
            bridge.idle_.wait(lock, [&bridge] {
                return bridge.in_flight_ == 0;
            });
        }
    };

    inline py::object py_log_level(const py::module_& logging, const utils::LogSink::Level level) {
        switch (level) {
        case utils::LogSink::Level::Debug:
            return logging.attr("DEBUG");
        case utils::LogSink::Level::Info:
            return logging.attr("INFO");
        case utils::LogSink::Level::Warning:
            return logging.attr("WARNING");
        case utils::LogSink::Level::Error:
            return logging.attr("ERROR");
        default:
            return logging.attr("NOTSET");
        }
    }

    inline void forward_log_message(const utils::LogSink::LogMessage& m) {
        auto& bridge = LogBridge::instance();
        if (Py_IsFinalizing() || !bridge.enter()) {
            return;
        }
        const LogBridge::Guard guard(bridge);
        const py::gil_scoped_acquire gil;
        try {
            const auto logging = py::module_::import("logging");
            const auto name = m.source == utils::LogSink::Source::Self ? "ntgcalls" : "webrtc";
            const auto message = m.file + ":" + std::to_string(m.line) + " " + m.message;
            (void) logging.attr("getLogger")(name).attr("log")(py_log_level(logging, m.level), message);
        } catch (py::error_already_set& e) {
            e.discard_as_unraisable("ntgcalls log bridge");
        }
    }

    inline void install_log_bridge() {
        {
            const py::gil_scoped_acquire gil;
            const auto logging = py::module_::import("logging");
            for (const char* name : {"webrtc", "ntgcalls"}) {
                if (auto logger = logging.attr("getLogger")(name); logger.attr("level").equal(logging.attr("NOTSET"))) {
                    logger.attr("setLevel")(logging.attr("CRITICAL"));
                }
            }
            py::module_::import("atexit").attr("register")(py::cpp_function(&LogBridge::close));
        }
        utils::LogSink::register_logger(forward_log_message);
    }
} // ntgcalls::support
