//
// Created by Laky64 on 12/08/2023.
//
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ntgcalls/ntgcalls.hpp>
#include <ntgcalls/exceptions.hpp>

namespace py = pybind11;

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

PYBIND11_MODULE(ntgcalls, m) {
    py::class_<ntgcalls::NTgCalls> wrapper(m, "NTgCalls");
    wrapper.def(py::init<>());
    wrapper.def("create_p2p_call", &ntgcalls::NTgCalls::createP2PCall, py::arg("user_id"), py::arg("media"));
    wrapper.def("init_exchange", &ntgcalls::NTgCalls::initExchange, py::arg("user_id"), py::arg("dh_config"), py::arg("g_a_hash"));
    wrapper.def("exchange_keys", &ntgcalls::NTgCalls::exchangeKeys, py::arg("user_id"), py::arg("g_a_or_b"), py::arg("fingerprint"));
    wrapper.def("skip_exchange", &ntgcalls::NTgCalls::skipExchange, py::arg("user_id"), py::arg("encryption_key"), py::arg("is_outgoing"));
    wrapper.def("connect_p2p", &ntgcalls::NTgCalls::connectP2P, py::arg("user_id"), py::arg("servers"), py::arg("versions"), py::arg("p2p_allowed"));
    wrapper.def("send_signaling", &ntgcalls::NTgCalls::sendSignalingData, py::arg("chat_id"), py::arg("msg_key"));
    wrapper.def("create_call", &ntgcalls::NTgCalls::createCall, py::arg("chat_id"), py::arg("media"));
    wrapper.def("connect", &ntgcalls::NTgCalls::connect, py::arg("chat_id"), py::arg("params"));
    wrapper.def("set_stream_sources", &ntgcalls::NTgCalls::setStreamSources, py::arg("chat_id"), py::arg("direction"), py::arg("media"));
    wrapper.def("pause", &ntgcalls::NTgCalls::pause, py::arg("chat_id"));
    wrapper.def("resume", &ntgcalls::NTgCalls::resume, py::arg("chat_id"));
    wrapper.def("mute", &ntgcalls::NTgCalls::mute, py::arg("chat_id"));
    wrapper.def("unmute", &ntgcalls::NTgCalls::unmute, py::arg("chat_id"));
    wrapper.def("stop", &ntgcalls::NTgCalls::stop, py::arg("chat_id"));
    wrapper.def("time", &ntgcalls::NTgCalls::time, py::arg("chat_id"), py::arg("direction"));
    wrapper.def("get_state", &ntgcalls::NTgCalls::getState, py::arg("chat_id"));
    wrapper.def("on_upgrade", &ntgcalls::NTgCalls::onUpgrade);
    wrapper.def("on_stream_end", &ntgcalls::NTgCalls::onStreamEnd);
    wrapper.def("on_connection_change", &ntgcalls::NTgCalls::onConnectionChange);
    wrapper.def("on_signaling", &ntgcalls::NTgCalls::onSignalingData, py::arg("callback"));
    wrapper.def("calls", &ntgcalls::NTgCalls::calls);
    wrapper.def("cpu_usage", &ntgcalls::NTgCalls::cpuUsage);
    wrapper.def_static("ping", &ntgcalls::NTgCalls::ping);
    wrapper.def_static("get_protocol", &ntgcalls::NTgCalls::getProtocol);
    wrapper.def_static("get_media_devices", &ntgcalls::NTgCalls::getMediaDevices);

    py::enum_<ntgcalls::StreamManager::Type>(m, "StreamType")
            .value("AUDIO", ntgcalls::StreamManager::Type::Audio)
            .value("VIDEO", ntgcalls::StreamManager::Type::Video)
            .export_values();

    py::enum_<ntgcalls::StreamManager::Mode>(m, "StreamMode")
            .value("RECORDING", ntgcalls::StreamManager::Mode::Recording)
            .value("PLAYBACK", ntgcalls::StreamManager::Mode::Playback)
            .export_values();

    py::enum_<ntgcalls::StreamManager::Device>(m, "StreamDevice")
            .value("MICROPHONE", ntgcalls::StreamManager::Device::Microphone)
            .value("SPEAKER", ntgcalls::StreamManager::Device::Speaker)
            .value("CAMERA", ntgcalls::StreamManager::Device::Camera)
            .value("SCREEN", ntgcalls::StreamManager::Device::Screen)
            .export_values();

    py::enum_<ntgcalls::StreamManager::Status>(m, "StreamStatus")
            .value("ACTIVE", ntgcalls::StreamManager::Status::Active)
            .value("PAUSED", ntgcalls::StreamManager::Status::Paused)
            .value("IDLING", ntgcalls::StreamManager::Status::Idling)
            .export_values();

    py::enum_<ntgcalls::CallInterface::ConnectionState>(m, "ConnectionState")
            .value("CONNECTING", ntgcalls::CallInterface::ConnectionState::Connecting)
            .value("CONNECTED", ntgcalls::CallInterface::ConnectionState::Connected)
            .value("FAILED", ntgcalls::CallInterface::ConnectionState::Failed)
            .value("TIMEOUT", ntgcalls::CallInterface::ConnectionState::Timeout)
            .value("CLOSED", ntgcalls::CallInterface::ConnectionState::Closed)
            .export_values();

    py::enum_<ntgcalls::BaseMediaDescription::InputMode>(m, "InputMode")
            .value("FILE", ntgcalls::BaseMediaDescription::InputMode::File)
            .value("SHELL", ntgcalls::BaseMediaDescription::InputMode::Shell)
            .value("FFMPEG", ntgcalls::BaseMediaDescription::InputMode::FFmpeg)
            .value("DEVICE", ntgcalls::BaseMediaDescription::InputMode::Device)
            .export_values()
            .def("__and__",[](const ntgcalls::BaseMediaDescription::InputMode& lhs, const ntgcalls::BaseMediaDescription::InputMode& rhs) {
                return static_cast<ntgcalls::BaseMediaDescription::InputMode>(lhs & rhs);
            })
            .def("__and__",[](const ntgcalls::BaseMediaDescription::InputMode& lhs, const int rhs) {
                return static_cast<ntgcalls::BaseMediaDescription::InputMode>(lhs & rhs);
            })
            .def("__or__",[](const ntgcalls::BaseMediaDescription::InputMode& lhs, const ntgcalls::BaseMediaDescription::InputMode& rhs) {
                return static_cast<ntgcalls::BaseMediaDescription::InputMode>(lhs | rhs);
            })
            .def("__or__",[](const ntgcalls::BaseMediaDescription::InputMode& lhs, const int rhs) {
                return static_cast<ntgcalls::BaseMediaDescription::InputMode>(lhs | rhs);
            });

    py::class_<ntgcalls::MediaState>(m, "MediaState")
            .def_readonly("muted", &ntgcalls::MediaState::muted)
            .def_readonly("video_stopped", &ntgcalls::MediaState::videoStopped)
            .def_readonly("video_paused", &ntgcalls::MediaState::videoPaused);

    py::class_<ntgcalls::StreamManager::MediaStatus>(m, "MediaStatus")
            .def_readonly("playback", &ntgcalls::StreamManager::MediaStatus::playback)
            .def_readonly("recording", &ntgcalls::StreamManager::MediaStatus::recording);

    py::class_<ntgcalls::DeviceInfo>(m, "DeviceInfo")
            .def_readonly("name", &ntgcalls::DeviceInfo::name)
            .def_readonly("metadata", &ntgcalls::DeviceInfo::metadata);

    py::class_<ntgcalls::MediaDevices>(m, "MediaDevices")
            .def_readonly("audio", &ntgcalls::MediaDevices::audio)
            .def_readonly("video", &ntgcalls::MediaDevices::video);

    py::class_<ntgcalls::BaseMediaDescription> mediaWrapper(m, "BaseMediaDescription");
    mediaWrapper.def_readwrite("input", &ntgcalls::BaseMediaDescription::input);

    py::class_<ntgcalls::AudioDescription> audioWrapper(m, "AudioDescription", mediaWrapper);
    audioWrapper.def(
            py::init<ntgcalls::BaseMediaDescription::InputMode, uint32_t, uint8_t, uint8_t, std::string>(),
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
            py::init<std::optional<ntgcalls::AudioDescription>, std::optional<ntgcalls::AudioDescription>, std::optional<ntgcalls::VideoDescription>, std::optional<ntgcalls::VideoDescription>>(),
            py::arg_v("microphone", std::nullopt, "None"),
            py::arg_v("speaker", std::nullopt, "None"),
            py::arg_v("camera", std::nullopt, "None"),
            py::arg_v("screen", std::nullopt, "None")
    );
    mediaDescWrapper.def_readwrite("microphone", &ntgcalls::MediaDescription::microphone);
    mediaDescWrapper.def_readwrite("speaker", &ntgcalls::MediaDescription::speaker);
    mediaDescWrapper.def_readwrite("camera", &ntgcalls::MediaDescription::camera);
    mediaDescWrapper.def_readwrite("screen", &ntgcalls::MediaDescription::screen);

    py::class_<ntgcalls::Protocol> protocolWrapper(m, "Protocol");
    protocolWrapper.def(py::init<>());
    protocolWrapper.def_readwrite("min_layer", &ntgcalls::Protocol::min_layer);
    protocolWrapper.def_readwrite("max_layer", &ntgcalls::Protocol::max_layer);
    protocolWrapper.def_readwrite("udp_p2p", &ntgcalls::Protocol::udp_p2p);
    protocolWrapper.def_readwrite("udp_reflector", &ntgcalls::Protocol::udp_reflector);
    protocolWrapper.def_readwrite("library_versions", &ntgcalls::Protocol::library_versions);

    py::class_<ntgcalls::RTCServer> rtcServerWrapper(m, "RTCServer");
    rtcServerWrapper.def(
        py::init<uint64_t, std::string, std::string, uint16_t, std::optional<std::string>,std::optional<std::string>, bool, bool, bool, std::optional<py::bytes>>(),
        py::arg("id"),
        py::arg("ipv4"),
        py::arg("ipv6"),
        py::arg("port"),
        py::arg("username") = std::nullopt,
        py::arg("password") = std::nullopt,
        py::arg("turn"),
        py::arg("stun"),
        py::arg("tcp"),
        py::arg("peer_tag") = std::nullopt
    );

    py::class_<ntgcalls::AuthParams> authParamsWrapper(m, "AuthParams");
    authParamsWrapper.def(py::init<>());
    authParamsWrapper.def_property_readonly("g_a_or_b", [](const ntgcalls::AuthParams& self) {
        return toBytes(self.g_a_or_b);
    });
    authParamsWrapper.def_readwrite("key_fingerprint", &ntgcalls::AuthParams::key_fingerprint);

    py::class_<ntgcalls::DhConfig> dhConfigWrapper(m, "DhConfig");
    dhConfigWrapper.def(py::init<int32_t, py::bytes, py::bytes>(), py::arg("g"), py::arg("p"), py::arg("random"));

    // Exceptions
    const pybind11::exception<wrtc::BaseRTCException> baseExc(m, "BaseRTCException");
    pybind11::register_exception<wrtc::SdpParseException>(m, "SdpParseException", baseExc);
    pybind11::register_exception<wrtc::RTCException>(m, "RTCException", baseExc);
    pybind11::register_exception<ntgcalls::ConnectionError>(m, "ConnectionError", baseExc);
    pybind11::register_exception<ntgcalls::TelegramServerError>(m, "TelegramServerError", baseExc);
    pybind11::register_exception<ntgcalls::ConnectionNotFound>(m, "ConnectionNotFound", baseExc);
    pybind11::register_exception<ntgcalls::InvalidParams>(m, "InvalidParams", baseExc);
    pybind11::register_exception<ntgcalls::RTMPNeeded>(m, "RTMPNeeded", baseExc);
    pybind11::register_exception<ntgcalls::FileError>(m, "FileError", baseExc);
    pybind11::register_exception<ntgcalls::FFmpegError>(m, "FFmpegError", baseExc);
    pybind11::register_exception<ntgcalls::ShellError>(m, "ShellError", baseExc);
    pybind11::register_exception<ntgcalls::CryptoError>(m, "CryptoError", baseExc);
    pybind11::register_exception<ntgcalls::SignalingError>(m, "SignalingError", baseExc);
    pybind11::register_exception<ntgcalls::MediaDeviceError>(m, "MediaDeviceError", baseExc);
    pybind11::register_exception<ntgcalls::SignalingUnsupported>(m, "SignalingUnsupported", baseExc);

    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
}