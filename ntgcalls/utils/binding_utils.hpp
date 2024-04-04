//
// Created by Laky64 on 19/03/2024.
//

#pragma once

#define WORKER(funcName, worker, ...) \
RTC_LOG(LS_INFO) << funcName << ": " << "Starting worker"; \
worker->PostTask([__VA_ARGS__] {\
RTC_LOG(LS_INFO) << funcName << ": " << "Worker started";

#define END_WORKER \
RTC_LOG(LS_INFO) << "Worker finished";\
});

#ifdef PYTHON_ENABLED
#include "wrtc/utils/binary.hpp"
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;

template <typename T, typename = std::enable_if_t<std::is_same_v<T, bytes::vector> || std::is_same_v<T, bytes::binary>>>
std::optional<T> toCBytes(const std::optional<py::bytes>& p) {
    if (p) {
        const auto data = reinterpret_cast<const uint8_t*>(PYBIND11_BYTES_AS_STRING(p->ptr()));
        const auto size = static_cast<size_t>(PYBIND11_BYTES_SIZE(p->ptr()));
        auto sharedPtr = T(size);
        std::memcpy(sharedPtr.data(), data, size);
        return sharedPtr;
    }
    return std::nullopt;
}

template <typename T, typename = std::enable_if_t<std::is_same_v<T, bytes::vector> || std::is_same_v<T, bytes::binary>>>
T toCBytes(const py::bytes& p) {
    const auto data = reinterpret_cast<const uint8_t*>(PYBIND11_BYTES_AS_STRING(p.ptr()));
    const auto size = static_cast<size_t>(PYBIND11_BYTES_SIZE(p.ptr()));
    auto sharedPtr = T(size);
    std::memcpy(sharedPtr.data(), data, size);
    return sharedPtr;
}

template <typename T, typename = std::enable_if_t<std::is_same_v<T, bytes::vector> || std::is_same_v<T, bytes::binary>>>
py::bytes toBytes(const T& p) {
    return {reinterpret_cast<const char*>(p.data()), p.size()};
}

py::object translate_current_exception();

#define THREAD_SAFE { \
py::gil_scoped_acquire acquire;

#define BYTES(x) py::bytes

#define CPP_BYTES(x, type) toCBytes<type>(x)

#define CAST_BYTES(...) toBytes(__VA_ARGS__)

#define ASYNC_RETURN(...) py::object

#define ASYNC_ARGS py::object loop;

#define INIT_ASYNC loop = py::module::import("asyncio").attr("get_event_loop")();

#define SMART_ASYNC(worker, ...) \
auto promise = loop.attr("create_future")(); \
auto functionName = __FUNCTION__;\
WORKER(functionName, worker, functionName, promise = promise.inc_ref(), __VA_ARGS__)\
try {

#define CLOSE_ASYNC(...) \
RTC_LOG(LS_VERBOSE) << "Acquiring GIL for setResult"; \
py::gil_scoped_acquire acquire;\
RTC_LOG(LS_VERBOSE) << "GIL acquired for setResult";\
loop.attr("call_soon_threadsafe")(promise.attr("set_result"), __VA_ARGS__); \
promise.dec_ref();\
} catch (const std::exception& e) {\
RTC_LOG(LS_VERBOSE) << "Acquiring GIL for setException"; \
py::gil_scoped_acquire acquire;\
RTC_LOG(LS_VERBOSE) << "GIL acquired for setException";\
loop.attr("call_soon_threadsafe")(promise.attr("set_exception"), translate_current_exception());\
promise.dec_ref();\
}\
END_WORKER \
return promise;

#define END_ASYNC CLOSE_ASYNC(nullptr)

#define END_ASYNC_RETURN_SAFE(...) CLOSE_ASYNC(__VA_ARGS__)

#define END_ASYNC_RETURN(...) \
auto result = __VA_ARGS__; \
CLOSE_ASYNC(result)

#else
#include <functional>
#include <rtc_base/thread.h>

template <typename T>
class AsyncPromise {
    rtc::Thread* worker;
    std::function<T()> callable;

public:
    AsyncPromise(rtc::Thread* worker, std::function<T()> callable): worker(worker), callable(std::move(callable)) {}

    void then(const std::function<void(T)>& resolve, const std::function<void(const std::exception_ptr&)>& reject) {
        worker->PostTask([this, resolve, reject, callable = callable]{
            try {
                resolve(callable());
            } catch (const std::exception&) {
                reject(std::current_exception());
            }
        });
    }
};

template <>
class AsyncPromise<void> {
    rtc::Thread* worker;
    std::function<void()> callable;
public:
    AsyncPromise(rtc::Thread* worker, std::function<void()> callable): worker(worker), callable(std::move(callable)) {};

    void then(const std::function<void()>& resolve, const std::function<void(const std::exception_ptr&)>& reject) const{
        worker->PostTask([this, resolve, reject, callable = callable]{
            try {
                callable();
                resolve();
            } catch (const std::exception&) {
                reject(std::current_exception());
            }
        });
    }
};

#define INIT_ASYNC

#define ASYNC_ARGS

#define THREAD_SAFE {

#define BYTES(x) x

#define CPP_BYTES(x, type) x

#define CAST_BYTES(...) __VA_ARGS__

#define ASYNC_FUNC_ARGS(...)

#define ASYNC_RETURN(...) AsyncPromise<__VA_ARGS__>

#define SMART_ASYNC(worker, ...) return { worker.get(), [__VA_ARGS__]{

#define CLOSE_ASYNC(...) }};

#define END_ASYNC CLOSE_ASYNC()
#define END_ASYNC_RETURN(...) \
return __VA_ARGS__;\
CLOSE_ASYNC()
#define END_ASYNC_RETURN_SAFE(...) END_ASYNC_RETURN(__VA_ARGS__)

#endif

#define END_THREAD_SAFE }