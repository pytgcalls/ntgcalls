//
// Created by Laky64 on 12/08/2023.
//
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ntgcalls.hpp"

namespace py = pybind11;

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

PYBIND11_MODULE(ntgcalls, m) {
    py::class_<ntgcalls::NTgCalls> wrapper(m, "NTgCalls");
    wrapper.def(py::init<>());
    wrapper.def("createCall",  &ntgcalls::NTgCalls::createCall, py::arg("chat_id"), py::arg("media"));
    wrapper.def("connect",  &ntgcalls::NTgCalls::connect, py::arg("chat_id"), py::arg("params"));
    wrapper.def("changeStream",  &ntgcalls::NTgCalls::changeStream, py::arg("chat_id"), py::arg("media"));
    wrapper.def("pause",  &ntgcalls::NTgCalls::pause, py::arg("chat_id"));
    wrapper.def("resume",  &ntgcalls::NTgCalls::resume, py::arg("chat_id"));
    wrapper.def("mute",  &ntgcalls::NTgCalls::mute, py::arg("chat_id"));
    wrapper.def("unmute",  &ntgcalls::NTgCalls::unmute, py::arg("chat_id"));
    wrapper.def("stop",  &ntgcalls::NTgCalls::stop, py::arg("chat_id"));
    wrapper.def("time",  &ntgcalls::NTgCalls::time, py::arg("chat_id"));

    py::class_<ntgcalls::FFmpegOptions> ffmpegWrapper(m, "FFmpegOptions");
    ffmpegWrapper.def(
            py::init<uint8_t>(),
            py::arg("streamId") = 0
    );
    ffmpegWrapper.def_readwrite("streamId", &ntgcalls::FFmpegOptions::streamId);

    py::class_<ntgcalls::AudioDescription> audioWrapper(m, "AudioDescription");
    audioWrapper.def(
            py::init<uint16_t, uint8_t, uint8_t, std::string, ntgcalls::FFmpegOptions>(),
            py::arg("sampleRate"),
            py::arg("bitsPerSample"),
            py::arg("channelCount"),
            py::arg("path"),
            py::arg_v("options", std::nullopt, "None")
    );
    audioWrapper.def_readwrite("sampleRate", &ntgcalls::AudioDescription::sampleRate);
    audioWrapper.def_readwrite("bitsPerSample", &ntgcalls::AudioDescription::bitsPerSample);
    audioWrapper.def_readwrite("channelCount", &ntgcalls::AudioDescription::channelCount);
    audioWrapper.def_readwrite("path", &ntgcalls::AudioDescription::path);
    audioWrapper.def_readwrite("options", &ntgcalls::AudioDescription::options);

    py::class_<ntgcalls::VideoDescription> videoWrapper(m, "VideoDescription");
    videoWrapper.def(
            py::init<uint16_t, uint16_t, uint8_t, std::string, ntgcalls::FFmpegOptions>(),
            py::arg("width"),
            py::arg("height"),
            py::arg("fps"),
            py::arg("path"),
            py::arg_v("options", std::nullopt, "None")
    );
    videoWrapper.def_readwrite("width", &ntgcalls::VideoDescription::width);
    videoWrapper.def_readwrite("height", &ntgcalls::VideoDescription::height);
    videoWrapper.def_readwrite("fps", &ntgcalls::VideoDescription::fps);
    videoWrapper.def_readwrite("path", &ntgcalls::VideoDescription::path);
    videoWrapper.def_readwrite("options", &ntgcalls::VideoDescription::options);

    py::class_<ntgcalls::MediaDescription> mediaWrapper(m, "MediaDescription");
    mediaWrapper.def(
            py::init<std::string, std::optional<ntgcalls::AudioDescription>, std::optional<ntgcalls::VideoDescription>>(),
            py::arg("encoder"),
            py::arg_v("audio", std::nullopt, "None"),
            py::arg_v("video", std::nullopt, "None")
    );
    mediaWrapper.def_readwrite("encoder", &ntgcalls::MediaDescription::encoder);
    mediaWrapper.def_readwrite("audio", &ntgcalls::MediaDescription::audio);
    mediaWrapper.def_readwrite("video", &ntgcalls::MediaDescription::video);

    // Exceptions
    pybind11::exception<wrtc::BaseRTCException> baseExc(m, "BaseRTCException");
    pybind11::register_exception<wrtc::SdpParseException>(m, "SdpParseException", baseExc);
    pybind11::register_exception<wrtc::RTCException>(m, "RTCException", baseExc);
    pybind11::register_exception<ntgcalls::ConnectionError>(m, "ConnectionError", baseExc);
    pybind11::register_exception<ntgcalls::InvalidParams>(m, "InvalidParams", baseExc);
    pybind11::register_exception<ntgcalls::RTMPNeeded>(m, "RTMPNeeded", baseExc);
    pybind11::register_exception<ntgcalls::FileError>(m, "FileError", baseExc);

    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
}