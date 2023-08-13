//
// Created by Laky64 on 12/08/2023.
//
#include <pybind11/pybind11.h>
#include <wrtc/wrtc.hpp>
#include "ntgcalls.hpp"

namespace py = pybind11;

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

using ntgcalls::NTgCalls;

PYBIND11_MODULE(ntgcalls, m) {
    py::class_<NTgCalls> wrapper(m, "NTgCalls");
    wrapper.def(py::init<>());
    wrapper.def("createCall",  &NTgCalls::createCall); // Needed python return annotation
    wrapper.def("setRemoteCallParams", &NTgCalls::setRemoteCallParams); // Needed python return annotation

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}