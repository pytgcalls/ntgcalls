//
// Python binding support -- generated bindings only, not part of the core lib.
//
#pragma once
#include <optional>
#include <string>
#include <vector>
#include <pybind11/pybind11.h>
#include <wrtc/utils/binary.hpp>

namespace py = pybind11;

namespace ntgcalls::support {
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, bytes::binary>>>
    T to_c_bytes(const py::bytes& p) {
        const std::string s = p;
        return T(s.begin(), s.end());
    }

    template <typename T, typename = std::enable_if_t<std::is_same_v<T, bytes::binary>>>
    std::optional<T> to_c_bytes(const std::optional<py::bytes>& p) {
        if (!p.has_value()) {
            return std::nullopt;
        }
        return to_c_bytes<T>(*p);
    }

    template <typename T, typename = std::enable_if_t<std::is_same_v<T, bytes::binary>>>
    std::vector<T> to_c_bytes(const std::vector<py::bytes>& p) {
        std::vector<T> result;
        result.reserve(p.size());
        for (const auto& item : p) {
            result.push_back(to_c_bytes<T>(item));
        }
        return result;
    }

    template <typename T>
    py::bytes to_bytes(const T& p) {
        return {reinterpret_cast<const char*>(p.data()), p.size()};
    }

    template <typename T>
    py::object to_bytes(const std::optional<T>& p) {
        if (!p.has_value()) {
            return py::none();
        }
        return to_bytes(*p);
    }
} // ntgcalls::support
