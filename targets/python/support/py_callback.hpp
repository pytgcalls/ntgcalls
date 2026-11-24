//
// Python binding support -- generated bindings only, not part of the core lib.
//
// Callbacks are invoked from C++ threads. During interpreter finalization
// acquiring the GIL from such a thread deletes its transient thread state
// and crashes in tstate_delete_common, so a callback fired while Python is
// shutting down is dropped, and an exception it raised is never released.
// The pybind function is shared, not copied: copying or destroying it takes
// the GIL, which C++ threads do whenever they copy the callback to invoke it.
//

#pragma once

#include <functional>
#include <memory>
#include <utility>

#include <py_log.hpp>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace ntgcalls::support {
    template<typename... Args>
    struct FinalizeSafeCallback {
        std::shared_ptr<std::function<void(Args...)>> callback;

        explicit FinalizeSafeCallback(std::function<void(Args...)> func): callback(new std::function(std::move(func)), release) {}

        void operator()(Args... args) const {
            if (Py_IsFinalizing()) {
                return;
            }
            try {
                (*callback)(std::forward<Args>(args)...);
            } catch (py::error_already_set& e) {
                if (Py_IsFinalizing()) {
                    (void) new py::error_already_set(std::move(e));
                    return;
                }
                throw;
            }
        }

    private:
        static void release(const std::function<void(Args...)>* func) {
            if (Py_IsFinalizing()) {
                return;
            }
            delete func;
        }
    };

    template<typename C, typename... Args>
    auto finalize_safe(void (C::*method)(const std::function<void(Args...)>&)) {
        return [method](C& self, std::function<void(Args...)> callback) {
            (self.*method)(FinalizeSafeCallback<Args...>(std::move(callback)));
        };
    }
} // ntgcalls::support
