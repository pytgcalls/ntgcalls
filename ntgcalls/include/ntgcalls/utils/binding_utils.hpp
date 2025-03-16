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
#include <wrtc/utils/binary.hpp>
#include <pybind11/pybind11.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <pybind11/stl.h>
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

#define ASYNC_ARGS \
py::object loop;\
py::object executor;

#define INIT_ASYNC \
loop = py::module::import("asyncio").attr("get_event_loop")();\
executor = py::module::import("concurrent.futures").attr("ThreadPoolExecutor")(std::min(static_cast<uint32_t>(32), std::thread::hardware_concurrency()));

#define DESTROY_ASYNC

#define SMART_ASYNC(...) \
return loop.attr("run_in_executor")(executor, py::cpp_function([__VA_ARGS__](){\
py::gil_scoped_release release;

#define END_ASYNC }));

#elif IS_ANDROID
#define INIT_ASYNC
#define DESTROY_ASYNC
#define ASYNC_ARGS
#define THREAD_SAFE {
#define BYTES(x) x
#define CPP_BYTES(x, type) x
#define CAST_BYTES(...) __VA_ARGS__
#define ASYNC_FUNC_ARGS(...)
#define ASYNC_RETURN(...) __VA_ARGS__
#define SMART_ASYNC(...)
#define END_ASYNC
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

#define INIT_ASYNC \
    asyncWorker = rtc::Thread::Create();\
    asyncWorker->Start();

#define DESTROY_ASYNC \
    asyncWorker->Stop();\
    asyncWorker = nullptr;

#define ASYNC_ARGS std::unique_ptr<rtc::Thread> asyncWorker;

#define THREAD_SAFE {

#define BYTES(x) x

#define CPP_BYTES(x, type) x

#define CAST_BYTES(...) __VA_ARGS__

#define ASYNC_FUNC_ARGS(...)

#define ASYNC_RETURN(...) AsyncPromise<__VA_ARGS__>

#define SMART_ASYNC(...) \
return { asyncWorker.get(), [__VA_ARGS__]{

#define END_ASYNC }};

#endif

#define END_THREAD_SAFE }