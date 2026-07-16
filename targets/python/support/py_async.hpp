//
// Python binding support -- generated bindings only, not part of the core lib.
//
#pragma once
#include <algorithm>
#include <thread>
#include <utility>
#include <pybind11/gil_safe_call_once.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace ntgcalls::support {
    inline py::object async_executor() {
        PYBIND11_CONSTINIT static py::gil_safe_call_once_and_store<py::object> storage;
        return storage.call_once_and_store_result([] {
            return py::module_::import("concurrent.futures")
                .attr("ThreadPoolExecutor")(std::min(32u, std::thread::hardware_concurrency()));
        }).get_stored();
    }

    template <typename F>
    py::object async_run(F fn) {
        const auto loop = py::module_::import("asyncio").attr("get_event_loop")();
        return loop.attr("run_in_executor")(async_executor(), py::cpp_function(std::move(fn)));
    }
} // ntgcalls::support