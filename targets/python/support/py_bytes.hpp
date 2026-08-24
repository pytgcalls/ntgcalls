//
// Python binding support -- generated bindings only, not part of the core lib.
//
#pragma once
#include <pybind11/pybind11.h>
#include <wrtc/utils/binary.hpp>

namespace py = pybind11;

template<>
class pybind11::detail::type_caster<bytes::binary> {
    PYBIND11_TYPE_CASTER(::bytes::binary, const_name("bytes"));

    bool load(const handle src, bool) {
        char* buffer = nullptr;
        Py_ssize_t length = 0;
        if (PyBytes_AsStringAndSize(src.ptr(), &buffer, &length) != 0) {
            PyErr_Clear();
            return false;
        }
        const auto data = reinterpret_cast<const ::bytes::byte*>(buffer);
        value.assign(data, data + length);
        return true;
    }

    static handle cast(const ::bytes::binary& src, return_value_policy, handle) {
        return PyBytes_FromStringAndSize(
            reinterpret_cast<const char*>(src.data()),
            static_cast<Py_ssize_t>(src.size())
        );
    }
};
