//
// Python binding support -- generated bindings only, not part of the core lib.
//
// A holder whose deleter releases the GIL before running the C++ destructor.
// The API object owns worker threads that call back into Python (e.g. the log
// bridge). Destroying it from Python joins those threads; if the GIL were held
// during the join, a worker blocked on gil_scoped_acquire would deadlock the
// join (ABBA). Releasing the GIL around the destructor lets the worker finish.
//

#pragma once

#include <memory>

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace ntgcalls::support {
    template<typename T>
    struct GilSafeDeleter {
        void operator()(const T* ptr) const {
            const py::gil_scoped_release release;
            delete ptr;
        }
    };

    template<typename T>
    using py_holder = std::unique_ptr<T, GilSafeDeleter<T>>;
} // ntgcalls::support
