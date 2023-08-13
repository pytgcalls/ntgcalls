//
// Created by Laky64 on 12/08/2023.
//
#include <pybind11/pybind11.h>
#include "client.hpp"

namespace py = pybind11;

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

PYBIND11_MODULE(ntgcalls, m) {
    py::class_<ntgcalls::Client> wrapper(m, "NTgCalls");
    wrapper.def(py::init<>());
    wrapper.def("createCall",  &ntgcalls::Client::createCall); // Needed python return annotation
    wrapper.def("setRemoteCallParams", &ntgcalls::Client::setRemoteCallParams); // Needed python return annotation

    // Exceptions
    pybind11::exception<wrtc::BaseRTCException> baseExc(m, "BaseRTCException");
    pybind11::register_exception<ntgcalls::ConnectionError>(m, "RTCException", baseExc);
    pybind11::register_exception<ntgcalls::InvalidParams>(m, "InvalidParams", baseExc);
    pybind11::register_exception<ntgcalls::RTMPNeeded>(m, "RTMPNeeded", baseExc);
    pybind11::register_exception<ntgcalls::FileError>(m, "FileError", baseExc);
#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}