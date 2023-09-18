//
// Created by Laky64 on 12/08/2023.
//
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "../ntgcalls.hpp"

namespace py = pybind11;

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

PYBIND11_MODULE(ntgcalls, m) {
    py::class_<ntgcalls::NTgCalls> wrapper(m, "NTgCalls");
    wrapper.def(py::init<>());
    wrapper.def("create_call", &ntgcalls::NTgCalls::createCall, py::arg("chat_id"), py::arg("media"));
    wrapper.def("connect", &ntgcalls::NTgCalls::connect, py::arg("chat_id"), py::arg("params"));
    wrapper.def("change_stream", &ntgcalls::NTgCalls::changeStream, py::arg("chat_id"), py::arg("media"));
    wrapper.def("pause", &ntgcalls::NTgCalls::pause, py::arg("chat_id"));
    wrapper.def("resume", &ntgcalls::NTgCalls::resume, py::arg("chat_id"));
    wrapper.def("mute", &ntgcalls::NTgCalls::mute, py::arg("chat_id"));
    wrapper.def("unmute", &ntgcalls::NTgCalls::unmute, py::arg("chat_id"));
    wrapper.def("stop", &ntgcalls::NTgCalls::stop, py::arg("chat_id"));
    wrapper.def("time", &ntgcalls::NTgCalls::time, py::arg("chat_id"));
    wrapper.def("get_state", &ntgcalls::NTgCalls::getState, py::arg("chat_id"));
    wrapper.def("on_upgrade", &ntgcalls::NTgCalls::onUpgrade);
    wrapper.def("on_stream_end", &ntgcalls::NTgCalls::onStreamEnd);
    wrapper.def("calls", &ntgcalls::NTgCalls::calls);
    wrapper.def_static("ping", &ntgcalls::NTgCalls::ping);

    py::enum_<ntgcalls::Stream::Type>(m, "StreamType")
            .value("Audio", ntgcalls::Stream::Type::Audio)
            .value("Video", ntgcalls::Stream::Type::Video)
            .export_values();

    py::enum_<ntgcalls::Stream::Status>(m, "StreamStatus")
            .value("Playing", ntgcalls::Stream::Status::Playing)
            .value("Paused", ntgcalls::Stream::Status::Paused)
            .value("Idling", ntgcalls::Stream::Status::Idling)
            .export_values();

    py::enum_<ntgcalls::BaseMediaDescription::InputMode>(m, "InputMode")
            .value("File", ntgcalls::BaseMediaDescription::InputMode::File)
            .value("Shell", ntgcalls::BaseMediaDescription::InputMode::Shell)
            .value("FFmpeg", ntgcalls::BaseMediaDescription::InputMode::FFmpeg)
            .export_values();

    py::class_<ntgcalls::MediaState>(m, "MediaState")
            .def_readonly("muted", &ntgcalls::MediaState::muted)
            .def_readonly("video_stopped", &ntgcalls::MediaState::videoStopped)
            .def_readonly("video_paused", &ntgcalls::MediaState::videoPaused);

    py::class_<ntgcalls::BaseMediaDescription> mediaWrapper(m, "BaseMediaDescription");
    mediaWrapper.def_readwrite("input", &ntgcalls::BaseMediaDescription::input);

    py::class_<ntgcalls::AudioDescription> audioWrapper(m, "AudioDescription", mediaWrapper);
    audioWrapper.def(
            py::init<ntgcalls::BaseMediaDescription::InputMode, uint16_t, uint8_t, uint8_t, std::string>(),
            py::arg("input_mode"),
            py::arg("sample_rate"),
            py::arg("bits_per_sample"),
            py::arg("channel_count"),
            py::arg("input")
    );
    audioWrapper.def_readwrite("sampleRate", &ntgcalls::AudioDescription::sampleRate);
    audioWrapper.def_readwrite("bitsPerSample", &ntgcalls::AudioDescription::bitsPerSample);
    audioWrapper.def_readwrite("channelCount", &ntgcalls::AudioDescription::channelCount);

    py::class_<ntgcalls::VideoDescription> videoWrapper(m, "VideoDescription", mediaWrapper);
    videoWrapper.def(
            py::init<ntgcalls::BaseMediaDescription::InputMode, uint16_t, uint16_t, uint8_t, std::string>(),
            py::arg("input_mode"),
            py::arg("width"),
            py::arg("height"),
            py::arg("fps"),
            py::arg("input")
    );
    videoWrapper.def_readwrite("width", &ntgcalls::VideoDescription::width);
    videoWrapper.def_readwrite("height", &ntgcalls::VideoDescription::height);
    videoWrapper.def_readwrite("fps", &ntgcalls::VideoDescription::fps);

    py::class_<ntgcalls::MediaDescription> mediaDescWrapper(m, "MediaDescription");
    mediaDescWrapper.def(
            py::init<std::optional<ntgcalls::AudioDescription>, std::optional<ntgcalls::VideoDescription>>(),
            py::arg_v("audio", std::nullopt, "None"),
            py::arg_v("video", std::nullopt, "None")
    );
    mediaDescWrapper.def_readwrite("audio", &ntgcalls::MediaDescription::audio);
    mediaDescWrapper.def_readwrite("video", &ntgcalls::MediaDescription::video);

    // Exceptions
    pybind11::exception<wrtc::BaseRTCException> baseExc(m, "BaseRTCException");
    pybind11::register_exception<wrtc::SdpParseException>(m, "SdpParseException", baseExc);
    pybind11::register_exception<wrtc::RTCException>(m, "RTCException", baseExc);
    pybind11::register_exception<ntgcalls::ConnectionError>(m, "ConnectionError", baseExc);
    pybind11::register_exception<ntgcalls::ConnectionNotFound>(m, "ConnectionNotFound", baseExc);
    pybind11::register_exception<ntgcalls::InvalidParams>(m, "InvalidParams", baseExc);
    pybind11::register_exception<ntgcalls::RTMPNeeded>(m, "RTMPNeeded", baseExc);
    pybind11::register_exception<ntgcalls::FileError>(m, "FileError", baseExc);
    pybind11::register_exception<ntgcalls::FFmpegError>(m, "FFmpegError", baseExc);
    pybind11::register_exception<ntgcalls::ShellError>(m, "ShellError", baseExc);

    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
}