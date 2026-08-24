//
// Python binding support -- generated bindings only, not part of the core lib.
//
#pragma once
#include <string>
#include <pybind11/pybind11.h>

namespace py = pybind11;

template<>
class pybind11::detail::type_caster<std::string> {
    PYBIND11_TYPE_CASTER(std::string, const_name("str"));

    bool load(const handle src, bool) {
        if (PyBytes_Check(src.ptr())) {
            char* buffer = nullptr;
            Py_ssize_t length = 0;
            if (PyBytes_AsStringAndSize(src.ptr(), &buffer, &length) != 0) {
                PyErr_Clear();
                return false;
            }
            value.assign(buffer, static_cast<size_t>(length));
            return true;
        }
        if (!PyUnicode_Check(src.ptr())) {
            return false;
        }
        auto* encoded = PyUnicode_AsEncodedString(src.ptr(), "utf-8", "surrogateescape");
        if (encoded == nullptr) {
            PyErr_Clear();
            return false;
        }
        value.assign(PyBytes_AS_STRING(encoded), static_cast<size_t>(PyBytes_GET_SIZE(encoded)));
        Py_DECREF(encoded);
        return true;
    }

    static handle cast(const std::string& src, return_value_policy, handle) {
        return PyUnicode_DecodeUTF8(src.data(), static_cast<Py_ssize_t>(src.size()), "surrogateescape");
    }
};
