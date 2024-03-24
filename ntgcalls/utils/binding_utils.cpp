//
// Created by Laky64 on 19/03/2024.
//
#include "binding_utils.hpp"
#ifdef PYTHON_ENABLED
#include <pybind11/embed.h>

py::object translate_current_exception() {
    const auto locals = py::dict();
    locals["rethrow_"] = py::cpp_function([]{throw;});
    exec(R"(
        try:
            rethrow_();
            result = None
        except Exception as e:
            result = e;
    )", py::globals(), locals);
    return locals["result"];
}
#else
template <typename T>
AsyncPromise<T>::AsyncPromise(rtc::Thread* worker, std::function<T()> callable): worker(worker), callable(std::move(callable)) {}

template <typename T>
void AsyncPromise<T>::then(const std::function<void(T)>& resolve, const std::function<void(const std::exception_ptr&)>& reject) {
    worker->PostTask([this, resolve, reject]{
        try {
            resolve(callable());
        } catch (const std::exception&) {
            reject(std::current_exception());
        }
    });
}
#endif
