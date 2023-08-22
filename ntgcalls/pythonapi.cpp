//
// Created by Laky64 on 12/08/2023.
//
#include <pybind11/pybind11.h>

#include "ntgcalls.hpp"

namespace py = pybind11;

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

PYBIND11_MODULE(ntgcalls, m) {
    py::class_<ntgcalls::NTgCalls> wrapper(m, "NTgCalls");
    wrapper.def(py::init<>());
    wrapper.def("init",  &ntgcalls::NTgCalls::init); // Needed python return annotation

    // Exceptions
    pybind11::exception<wrtc::BaseRTCException> baseExc(m, "BaseRTCException");
    pybind11::register_exception<ntgcalls::ConnectionError>(m, "RTCException", baseExc);
    pybind11::register_exception<ntgcalls::InvalidParams>(m, "InvalidParams", baseExc);
    pybind11::register_exception<ntgcalls::RTMPNeeded>(m, "RTMPNeeded", baseExc);
    pybind11::register_exception<ntgcalls::FileError>(m, "FileError", baseExc);

    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
}