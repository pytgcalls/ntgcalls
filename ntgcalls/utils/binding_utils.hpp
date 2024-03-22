//
// Created by Laky64 on 19/03/2024.
//

#pragma once

#include "wrtc/utils/binary.hpp"
#ifdef PYTHON_ENABLED
#include <pybind11/pybind11.h>
namespace py = pybind11;
#endif

#ifdef PYTHON_ENABLED
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

#define THREAD_SAFE { \
py::gil_scoped_acquire acquire;

#define BYTES(x) py::bytes

#define CPP_BYTES(x, type) toCBytes<type>(x)

#define CAST_BYTES(...) toBytes(__VA_ARGS__)

#define ASYNC_RETURN(...) py::object

#define ASYNC_ARGS py::object loop;

#define INIT_ASYNC loop = py::module::import("asyncio").attr("get_event_loop")();

#define SMART_ASYNC(...) \
auto promise = loop.attr("create_future")(); \
py::gil_scoped_release release;\
std::thread([promise = promise.inc_ref(), __VA_ARGS__] {\
try {

#define CLOSE_ASYNC(...) \
py::gil_scoped_acquire acquire;\
loop.attr("call_soon_threadsafe")(promise.attr("set_result"), __VA_ARGS__); \
} catch (const std::exception& e) {\
}\
}).detach();\
return promise;

#define END_ASYNC CLOSE_ASYNC(nullptr)

#define END_ASYNC_RETURN_SAFE(...) CLOSE_ASYNC(__VA_ARGS__)

#define END_ASYNC_RETURN(...) \
auto result = __VA_ARGS__; \
CLOSE_ASYNC(result)

#else

#define INIT_ASYNC

#define ASYNC_ARGS

#define THREAD_SAFE {

#define BYTES(x) x

#define CPP_BYTES(x, type) x

#define CAST_BYTES(...) __VA_ARGS__

#define ASYNC_RETURN(...) __VA_ARGS__

#define SMART_ASYNC(...)

#define CLOSE_ASYNC(...)
#define END_ASYNC CLOSE_ASYNC()
#define END_ASYNC_RETURN(...)
#define END_ASYNC_RETURN_SAFE(...)

#endif

#define END_THREAD_SAFE }