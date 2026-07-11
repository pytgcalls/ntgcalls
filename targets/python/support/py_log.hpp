//
// Python binding support -- generated bindings only, not part of the core lib.
//
#pragma once
#include <string>
#include <ntgcalls/utils/log_sink_impl.hpp>
#include <pybind11/pybind11.h>

#if PY_VERSION_HEX < 0x030D0000
#define Py_IsFinalizing _Py_IsFinalizing
#endif

namespace py = pybind11;

namespace ntgcalls::support {
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

    inline void install_log_bridge() {
        {
            const py::gil_scoped_acquire gil;
            const auto logging = py::module_::import("logging");
            for (const char* name : {"webrtc", "ntgcalls"}) {
                if (auto logger = logging.attr("getLogger")(name); logger.attr("level").equal(logging.attr("NOTSET"))) {
                    logger.attr("setLevel")(logging.attr("CRITICAL"));
                }
            }
        }
        utils::LogSink::register_logger([](const utils::LogSink::LogMessage &m) {
            if (Py_IsFinalizing()) {
                return;
            }
            const py::gil_scoped_acquire gil;
            const auto logging = py::module_::import("logging");
            const auto name = m.source == utils::LogSink::Source::Self ? "ntgcalls" : "webrtc";
            const auto message = m.file + ":" + std::to_string(m.line) + " " + m.message;
            (void) logging.attr("getLogger")(name).attr("log")(py_log_level(logging, m.level), message);
        });
    }
} // ntgcalls::support