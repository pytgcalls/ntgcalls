//
// Created by Laky64 on 29/08/2023.
//

#include <ntgcalls/ntgcalls.hpp>
#include "ntgcalls.h"
#include <ntgcalls/exceptions.hpp>

constexpr uint16_t SUPPORTED_INPUTS = NTG_FILE | NTG_SHELL | NTG_DEVICE | NTG_DESKTOP | NTG_EXTERNAL;

#define REGISTER_EXCEPTION(x, y) \
} catch (const x& msg) { \
    *future.errorCode = NTG_ERROR_##y; \
    *future.errorMessage = strdup(msg.what());

#define HANDLE_EXCEPTIONS [future](const std::exception_ptr& e) {\
try { \
std::rethrow_exception(e); \
REGISTER_EXCEPTION(ntgcalls::ConnectionNotFound, CONNECTION_NOT_FOUND) \
REGISTER_EXCEPTION(ntgcalls::ConnectionError, CONNECTION) \
REGISTER_EXCEPTION(ntgcalls::TelegramServerError, TELEGRAM_SERVER) \
REGISTER_EXCEPTION(ntgcalls::SignalingUnsupported, SIGNALING_UNSUPPORTED) \
REGISTER_EXCEPTION(ntgcalls::SignalingError, SIGNALING) \
REGISTER_EXCEPTION(ntgcalls::InvalidParams, INVALID_PARAMS) \
REGISTER_EXCEPTION(ntgcalls::CryptoError, CRYPTO) \
REGISTER_EXCEPTION(ntgcalls::FileError, FILE) \
REGISTER_EXCEPTION(ntgcalls::FFmpegError, FFMPEG) \
REGISTER_EXCEPTION(ntgcalls::ShellError, SHELL) \
REGISTER_EXCEPTION(ntgcalls::MediaDeviceError, MEDIA_DEVICE) \
REGISTER_EXCEPTION(ntgcalls::RTMPStreamingUnsupported, RTMP_STREAMING_UNSUPPORTED) \
REGISTER_EXCEPTION(ntgcalls::RTCConnectionNeeded, RTC_CONNECTION_NEEDED) \
REGISTER_EXCEPTION(wrtc::RTCException, WEBRTC) \
REGISTER_EXCEPTION(wrtc::SdpParseException, PARSE_SDP) \
REGISTER_EXCEPTION(wrtc::TransportParseException, PARSE_TRANSPORT) \
} catch (...) { \
*future.errorCode = NTG_ERROR_UNKNOWN; \
} \
future.promise(future.userData);\
}\

#define PREPARE_ASYNC(method, ...) \
*future.errorCode = NTG_ERROR_ASYNC_NOT_READY;\
try {\
getInstance(ptr)->method(__VA_ARGS__).then(\

#define PREPARE_ASYNC_END \
, HANDLE_EXCEPTIONS \
);\
} catch (ntgcalls::NullPointer&) {\
return NTG_ERROR_NULL_POINTER;\
} catch (...) {\
return NTG_ERROR_UNKNOWN;\
}\
return 0;

void copyAndReturn(const std::vector<std::byte>& b, uint8_t* buffer, const int size) {
    if (!buffer)
        return;
    if (size < static_cast<int>(b.size()))
        return;
    const auto *bufferTemp = reinterpret_cast<const uint8_t*>(b.data());
    std::copy_n(bufferTemp, b.size(), buffer);
}

int copyAndReturn(const std::vector<std::byte>& b, uint8_t** buffer, int* size) {
    if (!buffer || !size)
        return NTG_ERROR_NULL_POINTER;
    auto* output = new uint8_t[b.size()];
    const auto *bufferTemp = reinterpret_cast<const uint8_t*>(b.data());
    std::copy_n(bufferTemp, b.size(), output);
    *size = static_cast<int>(b.size());
    *buffer = output;
    return 0;
}

int copyAndReturn(std::string s, char** buffer) {
    if (!buffer)
        return NTG_ERROR_NULL_POINTER;
    auto *output = new char[s.size() + 1];
    std::ranges::copy(s, output);
    output[s.size()] = '\0';
    *buffer = output;
    return 0;
}

template <typename T>
void copyAndReturn(std::vector<T> b, T **buffer, int* size) {
    if (!buffer || !size)
        return;
    auto *output = new T[b.size()];
    std::copy(b.begin(), b.end(), output);
    *buffer = output;
    *size = static_cast<int>(b.size());
}

bytes::vector copyAndReturn(const uint8_t* buffer, const int size) {
    bytes::vector b(size);
    const auto *bufferTemp = reinterpret_cast<const std::byte*>(buffer);
    std::copy_n(bufferTemp, size, b.begin());
    return b;
}

ntgcalls::NTgCalls* getInstance(const uintptr_t ptr) {
    const auto instance = reinterpret_cast<ntgcalls::NTgCalls*>(ptr);
    if (!instance) {
        throw ntgcalls::NullPointer("NullPointer exception for " + std::to_string(ptr));
    }
    return instance;
}

ntgcalls::BaseMediaDescription::MediaSource parseMediaSource(const ntg_media_source_enum mode) {
    switch (mode) {
        case NTG_FILE:
            return ntgcalls::BaseMediaDescription::MediaSource::File;
        case NTG_SHELL:
            return ntgcalls::BaseMediaDescription::MediaSource::Shell;
        case NTG_FFMPEG:
            return ntgcalls::BaseMediaDescription::MediaSource::FFmpeg;
        case NTG_DEVICE:
            return ntgcalls::BaseMediaDescription::MediaSource::Device;
        case NTG_DESKTOP:
            return ntgcalls::BaseMediaDescription::MediaSource::Desktop;
        case NTG_EXTERNAL:
            return ntgcalls::BaseMediaDescription::MediaSource::External;
        default:
            return ntgcalls::BaseMediaDescription::MediaSource::Unknown;
    }
}

ntg_media_state_struct parseCMediaState(const ntgcalls::MediaState state) {
    return ntg_media_state_struct{
        state.muted,
        state.videoPaused,
        state.videoStopped,
        state.presentationPaused
    };
}

ntg_stream_status_enum parseCStatus(const ntgcalls::StreamManager::Status status) {
    switch (status) {
    case ntgcalls::StreamManager::Status::Active:
        return NTG_ACTIVE;
    case ntgcalls::StreamManager::Status::Paused:
        return NTG_PAUSED;
    case ntgcalls::StreamManager::Status::Idling:
        return NTG_IDLING;
    }
    return {};
}

ntg_log_level_enum parseCLevel(const ntgcalls::LogSink::Level level) {
    switch (level) {
        case ntgcalls::LogSink::Level::Debug:
            return NTG_LOG_DEBUG;
        case ntgcalls::LogSink::Level::Info:
            return NTG_LOG_INFO;
        case ntgcalls::LogSink::Level::Warning:
            return NTG_LOG_WARNING;
        case ntgcalls::LogSink::Level::Error:
            return NTG_LOG_ERROR;
        default:
            return NTG_LOG_UNKNOWN;
    }
}

ntg_connection_state_enum parseCConnectionState(const ntgcalls::NetworkInfo::ConnectionState state) {
    switch (state) {
        case ntgcalls::NetworkInfo::ConnectionState::Connecting:
            return NTG_STATE_CONNECTING;
        case ntgcalls::NetworkInfo::ConnectionState::Connected:
            return NTG_STATE_CONNECTED;
        case ntgcalls::NetworkInfo::ConnectionState::Timeout:
            return NTG_STATE_TIMEOUT;
        case ntgcalls::NetworkInfo::ConnectionState::Failed:
            return NTG_STATE_FAILED;
        case ntgcalls::NetworkInfo::ConnectionState::Closed:
            return NTG_STATE_CLOSED;
    }
    return {};
}

ntg_network_info_struct parseCNetworkInfo(const ntgcalls::NetworkInfo state) {
    return {
        state.kind == ntgcalls::NetworkInfo::Kind::Normal? NTG_KIND_NORMAL : NTG_KIND_PRESENTATION,
        parseCConnectionState(state.state),
    };
}

ntgcalls::DhConfig parseDhConfig(ntg_dh_config_struct* config) {
    return {
        config->g,
        copyAndReturn(config->p, config->sizeP),
        copyAndReturn(config->random, config->sizeRandom)
    };
}

ntg_log_source_enum parseCSource(const ntgcalls::LogSink::Source source) {
    switch (source) {
        case ntgcalls::LogSink::Source::WebRTC:
            return NTG_LOG_WEBRTC;
        case ntgcalls::LogSink::Source::Self:
            return NTG_LOG_SELF;
    }
    return {};
}

std::vector<ntgcalls::RTCServer> parseRTCServers(ntg_rtc_server_struct* servers, const int size) {
    std::vector<ntgcalls::RTCServer> serversCpp;
    serversCpp.reserve(size);
    for (int i = 0; i < size; i++) {
        serversCpp.emplace_back(
            servers[i].id,
            servers[i].ipv4,
            servers[i].ipv6,
            servers[i].port,
            servers[i].username,
            servers[i].password,
            servers[i].turn,
            servers[i].stun,
            servers[i].tcp,
            servers[i].peerTagSize? std::optional(bytes::binary(servers[i].peerTag, servers[i].peerTag + servers[i].peerTagSize)) : std::nullopt
        );
    }
    return serversCpp;
}

std::vector<std::string> copyAndReturn(char** versions, const int size) {
    std::vector<std::string> versionsCpp;
    versionsCpp.reserve(size);
    for (int i = 0; i < size; i++) {
        versionsCpp.emplace_back(versions[i]);
    }
    return versionsCpp;
}

std::pair<char**, int> copyAndReturn(const std::vector<std::string>& versions) {
    auto versionsCpp = new char*[versions.size()];
    for (int i = 0; i < versions.size(); i++) {
        copyAndReturn(versions[i], &versionsCpp[i]);
    }
    return {versionsCpp, static_cast<int>(versions.size())};
}

std::pair<ntg_device_info_struct*, int> copyAndReturn(const std::vector<ntgcalls::DeviceInfo>& devices) {
    auto cDevices = new ntg_device_info_struct[devices.size()];
    for (int i = 0; i < devices.size(); i++) {
        ntg_device_info_struct result{};
        copyAndReturn(devices[i].name, &result.name);
        copyAndReturn(devices[i].metadata, &result.metadata);
        cDevices[i] = result;
    }
    return {cDevices, static_cast<int>(devices.size())};
}

ntgcalls::AudioDescription parseAudioDescription(const ntg_audio_description_struct& desc) {
    if (desc.mediaSource & SUPPORTED_INPUTS) {
        return {
            parseMediaSource(desc.mediaSource),
            desc.sampleRate,
            desc.channelCount,
            std::string(desc.input)
        };
    }
    throw ntgcalls::FFmpegError("Not supported");
}

ntgcalls::VideoDescription parseVideoDescription(const ntg_video_description_struct& desc) {
    if (desc.mediaSource & SUPPORTED_INPUTS) {
        return {
            parseMediaSource(desc.mediaSource),
            desc.width,
            desc.height,
            desc.fps,
            std::string(desc.input)
        };
    }
    throw ntgcalls::FFmpegError("Not supported");
}

ntgcalls::MediaDescription parseMediaDescription(const ntg_media_description_struct& desc) {
    std::optional<ntgcalls::AudioDescription> microphone;
    std::optional<ntgcalls::AudioDescription> speaker;
    std::optional<ntgcalls::VideoDescription> camera;
    std::optional<ntgcalls::VideoDescription> screen;
    if (desc.microphone) {
        microphone = parseAudioDescription(*desc.microphone);
    }
    if (desc.speaker) {
        speaker = parseAudioDescription(*desc.speaker);
    }
    if (desc.camera) {
        camera = parseVideoDescription(*desc.camera);
    }
    if (desc.screen) {
        screen = parseVideoDescription(*desc.screen);
    }
    return ntgcalls::MediaDescription(
        microphone,
        speaker,
        camera,
        screen
    );
}

ntg_stream_mode_enum parseCStreamMode(const ntgcalls::StreamManager::Mode mode) {
    switch (mode) {
    case ntgcalls::StreamManager::Mode::Capture:
        return NTG_STREAM_CAPTURE;
    case ntgcalls::StreamManager::Mode::Playback:
        return NTG_STREAM_PLAYBACK;
    }
    return {};
}

ntgcalls::StreamManager::Mode parseStreamMode(const ntg_stream_mode_enum mode) {
    switch (mode) {
    case NTG_STREAM_CAPTURE:
        return ntgcalls::StreamManager::Mode::Capture;
    case NTG_STREAM_PLAYBACK:
        return ntgcalls::StreamManager::Mode::Playback;
    }
    return {};
}

ntg_stream_device_enum parseCStreamDevice(const ntgcalls::StreamManager::Device device) {
    switch (device) {
    case ntgcalls::StreamManager::Device::Microphone:
        return NTG_STREAM_MICROPHONE;
    case ntgcalls::StreamManager::Device::Speaker:
        return NTG_STREAM_SPEAKER;
    case ntgcalls::StreamManager::Device::Camera:
        return NTG_STREAM_CAMERA;
    case ntgcalls::StreamManager::Device::Screen:
        return NTG_STREAM_SCREEN;
    }
    return {};
}

ntgcalls::StreamManager::Device parseStreamDevice(const ntg_stream_device_enum device) {
    switch (device) {
    case NTG_STREAM_MICROPHONE:
        return ntgcalls::StreamManager::Device::Microphone;
    case NTG_STREAM_SPEAKER:
        return ntgcalls::StreamManager::Device::Speaker;
    case NTG_STREAM_CAMERA:
        return ntgcalls::StreamManager::Device::Camera;
    case NTG_STREAM_SCREEN:
        return ntgcalls::StreamManager::Device::Screen;
    }
    return {};
}

ntg_stream_type_enum parseCStreamType(const ntgcalls::StreamManager::Type type) {
    switch (type) {
    case ntgcalls::StreamManager::Type::Audio:
        return NTG_STREAM_AUDIO;
    case ntgcalls::StreamManager::Type::Video:
        return NTG_STREAM_VIDEO;
    }
    return {};
}

ntg_media_segment_quality_enum parseCSegmentQuality(const wrtc::MediaSegment::Quality quality) {
    switch (quality) {
    case wrtc::MediaSegment::Quality::None:
        return NTG_MEDIA_SEGMENT_QUALITY_NONE;
    case wrtc::MediaSegment::Quality::Thumbnail:
        return NTG_MEDIA_SEGMENT_QUALITY_THUMBNAIL;
    case wrtc::MediaSegment::Quality::Medium:
        return NTG_MEDIA_SEGMENT_QUALITY_MEDIUM;
    case wrtc::MediaSegment::Quality::Full:
        return NTG_MEDIA_SEGMENT_QUALITY_FULL;
    }
    return {};
}

wrtc::MediaSegment::Part::Status parseSegmentStatus(const ntg_media_segment_status_enum status) {
    switch (status) {
    case NTG_MEDIA_SEGMENT_NOT_READY:
        return wrtc::MediaSegment::Part::Status::NotReady;
    case NTG_MEDIA_SEGMENT_RESYNC_NEEDED:
        return wrtc::MediaSegment::Part::Status::ResyncNeeded;
    case NTG_MEDIA_SEGMENT_SUCCESS:
        return wrtc::MediaSegment::Part::Status::Success;
    }
    return {};
}

ntg_connection_mode_enum parseCConnectionMode(const wrtc::ConnectionMode mode) {
    switch (mode) {
    case wrtc::ConnectionMode::None:
        return NTG_CONNECTION_MODE_NONE;
    case wrtc::ConnectionMode::Rtc:
        return NTG_CONNECTION_MODE_RTC;
    case wrtc::ConnectionMode::Stream:
        return NTG_CONNECTION_MODE_STREAM;
    case wrtc::ConnectionMode::Rtmp:
        return NTG_CONNECTION_MODE_RTMP;
    }
    return {};
}

ntg_frame_data_struct parseCFrameData(const wrtc::FrameData data) {
    return {
        data.absoluteCaptureTimestampMs,
        data.width,
        data.height,
        static_cast<uint16_t>(data.rotation)
    };
}

wrtc::FrameData parseFrameData(const ntg_frame_data_struct data) {
    return {
        data.absoluteCaptureTimestampMs,
        static_cast<webrtc::VideoRotation>(data.rotation),
        data.height,
        data.width
    };
}

std::vector<wrtc::SsrcGroup> parseSsrcGroups(ntg_ssrc_group_struct* ssrcGroups, const int size) {
    std::vector<wrtc::SsrcGroup> groups;
    for (int i = 0; i < size; i++) {
        auto [semantics, ssrcs, sizeSsrcs] = ssrcGroups[i];
        groups.emplace_back(
            std::string(semantics),
            std::vector(ssrcs, ssrcs + sizeSsrcs)
        );
    }
    return groups;
}

uintptr_t ntg_init() {
    return reinterpret_cast<uintptr_t>(new ntgcalls::NTgCalls());
}

int ntg_destroy(const uintptr_t ptr) {
    try {
        delete getInstance(ptr);
        return 0;
    } catch (ntgcalls::NullPointer&) {
        return NTG_ERROR_NULL_POINTER;
    }
}

int ntg_init_presentation(const uintptr_t ptr, const int64_t chatId, char** buffer, ntg_async_struct future) {
    PREPARE_ASYNC(initPresentation, chatId)
    [future, buffer] (const std::string& s) {
        *future.errorCode = copyAndReturn(s, buffer);
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_stop_presentation(const uintptr_t ptr, const int64_t chatId, ntg_async_struct future) {
    PREPARE_ASYNC(stopPresentation, chatId)
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_add_incoming_video(const uintptr_t ptr, const int64_t chatId, char* endpoint, ntg_ssrc_group_struct* ssrcGroups, const int size, uint32_t* buffer, ntg_async_struct future) {
    PREPARE_ASYNC(addIncomingVideo, chatId, std::string(endpoint), parseSsrcGroups(ssrcGroups, size))
    [future, buffer](const uint32_t ssrc) {
        *future.errorCode = ssrc ? 0 : 1;
        *buffer = ssrc;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_remove_incoming_video(const uintptr_t ptr, const int64_t chatId, char* endpoint, ntg_async_struct future) {
    PREPARE_ASYNC(removeIncomingVideo, chatId, std::string(endpoint))
    [future](const bool success) {
        *future.errorCode = !success;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_create_p2p(const uintptr_t ptr, const int64_t userId, ntg_async_struct future) {
    PREPARE_ASYNC(createP2PCall, userId)
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_init_exchange(const uintptr_t ptr, const int64_t userId, ntg_dh_config_struct* dhConfig, const uint8_t* g_a_hash, const int sizeGAHash, uint8_t** buffer, int* size, ntg_async_struct future) {
    PREPARE_ASYNC(initExchange, userId, parseDhConfig(dhConfig), sizeGAHash ? std::optional(copyAndReturn(g_a_hash, sizeGAHash)) : std::nullopt)
    [future, buffer, size] (const bytes::vector& s){
        *future.errorCode = copyAndReturn(s, buffer, size);
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_exchange_keys(const uintptr_t ptr, const int64_t userId, const uint8_t* g_a_or_b, const int sizeGAB, const int64_t fingerprint, ntg_auth_params_struct* buffer, ntg_async_struct future) {
    PREPARE_ASYNC(exchangeKeys, userId, copyAndReturn(g_a_or_b, sizeGAB), fingerprint)
    [future, buffer](const ntgcalls::AuthParams& params) {
        buffer->key_fingerprint = params.key_fingerprint;
        buffer->g_a_or_b = new uint8_t[params.g_a_or_b.size()];
        buffer->sizeGAB = static_cast<int>(params.g_a_or_b.size());
        copyAndReturn(params.g_a_or_b, buffer->g_a_or_b, buffer->sizeGAB);
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_skip_exchange(const uintptr_t ptr, const int64_t userId, const uint8_t* encryptionKey, const int size, const bool isOutgoing, ntg_async_struct future) {
    PREPARE_ASYNC(skipExchange, userId, copyAndReturn(encryptionKey, size), isOutgoing)
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_connect_p2p(const uintptr_t ptr, const int64_t userId, ntg_rtc_server_struct* servers, const int serversSize, char** versions, const int versionsSize, const bool p2pAllowed, ntg_async_struct future) {
    PREPARE_ASYNC(connectP2P, userId, parseRTCServers(servers, serversSize), copyAndReturn(versions, versionsSize), p2pAllowed)
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_send_signaling_data(const uintptr_t ptr, const int64_t userId, uint8_t* buffer, const int size, ntg_async_struct future) {
    PREPARE_ASYNC(sendSignalingData, userId, bytes::binary(buffer, buffer + size))
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_get_protocol(ntg_protocol_struct* buffer) {
    const auto [min_layer, max_layer, udp_p2p, udp_reflector, library_versions] = ntgcalls::NTgCalls::getProtocol();
    buffer->minLayer = min_layer;
    buffer->maxLayer = max_layer;
    buffer->udpP2P = udp_p2p;
    buffer->udpReflector = udp_reflector;
    auto [libraryVersions, libraryVersionsSize] = copyAndReturn(library_versions);
    buffer->libraryVersionsSize = libraryVersionsSize;
    buffer->libraryVersions = libraryVersions;
    return 0;
}

int ntg_create(const uintptr_t ptr, const int64_t chatID, char** buffer, ntg_async_struct future) {
    PREPARE_ASYNC(createCall, chatID)
    [future, buffer](const std::string& s) {
        *future.errorCode = copyAndReturn(s, buffer);
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_connect(const uintptr_t ptr, const int64_t chatID, char* params, const bool isPresentation, ntg_async_struct future) {
    PREPARE_ASYNC(connect, chatID, std::string(params), isPresentation)
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

// ReSharper disable once CppPassValueParameterByConstReference
int ntg_set_stream_sources(const uintptr_t ptr, const int64_t chatID, const ntg_stream_mode_enum streamMode, ntg_media_description_struct desc, ntg_async_struct future) {
    PREPARE_ASYNC(setStreamSources, chatID, parseStreamMode(streamMode), parseMediaDescription(desc))
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_pause(const uintptr_t ptr, const int64_t chatID, ntg_async_struct future) {
    PREPARE_ASYNC(pause, chatID)
    [future](const bool success) {
        *future.errorCode = !success;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_resume(const uintptr_t ptr, const int64_t chatID, ntg_async_struct future) {
    PREPARE_ASYNC(resume, chatID)
    [future](const bool success) {
        *future.errorCode = !success;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_mute(const uintptr_t ptr, const int64_t chatID, ntg_async_struct future) {
    PREPARE_ASYNC(mute, chatID)
    [future](const bool success) {
        *future.errorCode = !success;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_unmute(const uintptr_t ptr, const int64_t chatID, ntg_async_struct future) {
    PREPARE_ASYNC(unmute, chatID)
    [future](const bool success) {
        *future.errorCode = !success;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_stop(const uintptr_t ptr, const int64_t chatID, ntg_async_struct future) {
    PREPARE_ASYNC(stop, chatID)
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_time(const uintptr_t ptr, const int64_t chatID, const ntg_stream_mode_enum streamMode, int64_t* time, ntg_async_struct future) {
    PREPARE_ASYNC(time, chatID, parseStreamMode(streamMode))
    [future, time](const int64_t t) {
        *time = t;
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_get_state(const uintptr_t ptr, const int64_t chatID, ntg_media_state_struct* mediaState, ntg_async_struct future) {
    PREPARE_ASYNC(getState, chatID)
    [future, mediaState](const ntgcalls::MediaState state) {
        *mediaState = parseCMediaState(state);
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_get_connection_mode(const uintptr_t ptr, const int64_t chatID, ntg_connection_mode_enum* mode, ntg_async_struct future) {
    PREPARE_ASYNC(getConnectionMode, chatID)
    [future, mode](const wrtc::ConnectionMode m) {
        *mode = parseCConnectionMode(m);
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_send_external_frame(const uintptr_t ptr, const int64_t chatID, const ntg_stream_device_enum device, uint8_t* frame, const int frameSize, const ntg_frame_data_struct frameData, ntg_async_struct future) {
    PREPARE_ASYNC(sendExternalFrame, chatID, parseStreamDevice(device), bytes::binary(frame, frame + frameSize), parseFrameData(frameData))
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_send_broadcast_timestamp(const uintptr_t ptr, const int64_t chatId, const int64_t timestamp, ntg_async_struct future) {
    PREPARE_ASYNC(sendBroadcastTimestamp, chatId, timestamp)
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_send_broadcast_part(const uintptr_t ptr, const int64_t chatId, const int64_t segmentId, const int32_t partId, const ntg_media_segment_status_enum status, const bool qualityUpdate, const uint8_t* frame, const int frameSize, ntg_async_struct future) {
    PREPARE_ASYNC(sendBroadcastPart, chatId, segmentId, partId, parseSegmentStatus(status), qualityUpdate, bytes::make_binary_optional(frame, frameSize))
    [future] {
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_get_media_devices(ntg_media_devices_struct* buffer) {
    auto [microphone, speaker, camera, screen] = ntgcalls::NTgCalls::getMediaDevices();
    auto [microphoneList, microphoneSize] = copyAndReturn(microphone);
    buffer->microphone = microphoneList;
    buffer->sizeMicrophone = microphoneSize;
    auto [speakerList, speakerSize] = copyAndReturn(speaker);
    buffer->speaker = speakerList;
    buffer->sizeSpeaker = speakerSize;
    auto [cameraList, cameraSize] = copyAndReturn(camera);
    buffer->camera = cameraList;
    buffer->sizeCamera = cameraSize;
    auto [screenList, screenSize] = copyAndReturn(screen);
    buffer->screen = screenList;
    buffer->sizeScreen = screenSize;
    return 0;
}

int ntg_calls(const uintptr_t ptr, ntg_call_info_struct** buffer, int* size, ntg_async_struct future) {
    PREPARE_ASYNC(calls)
    [future, buffer, size](const auto& callsCpp) {
        std::vector<ntg_call_info_struct> groupCalls;
        groupCalls.reserve(callsCpp.size());
        for (const auto [chatId, status] : callsCpp) {
            groupCalls.push_back(ntg_call_info_struct{
                chatId,
                parseCStatus(status.capture),
                parseCStatus(status.playback)
            });
        }
        copyAndReturn(groupCalls, buffer, size);
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_calls_count(const uintptr_t ptr, uint64_t* size, ntg_async_struct future) {
    PREPARE_ASYNC(calls)
    [future, size](const auto& callsCpp) {
        *size = callsCpp.size();
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_cpu_usage(const uintptr_t ptr, double* buffer, ntg_async_struct future) {
    PREPARE_ASYNC(cpuUsage)
    [future, buffer](const double usage) {
        *buffer = usage;
        *future.errorCode = 0;
        future.promise(future.userData);
    }
    PREPARE_ASYNC_END
}

int ntg_enable_g_lib_loop(const bool enable) {
    try {
        ntgcalls::NTgCalls::enableGlibLoop(enable);
    } catch (ntgcalls::MediaDeviceError&) {
        return NTG_ERROR_MEDIA_DEVICE;
    }
    return 0;
}

int ntg_on_stream_end(const uintptr_t ptr, ntg_stream_callback callback, void* userData) {
    try {
        getInstance(ptr)->onStreamEnd([ptr, callback, userData](const int64_t chatId, const ntgcalls::StreamManager::Type type, const ntgcalls::StreamManager::Device device) {
            callback(
                ptr,
                chatId,
                parseCStreamType(type),
                parseCStreamDevice(device),
                userData
            );
        });
    } catch (ntgcalls::NullPointer&) {
        return NTG_ERROR_NULL_POINTER;
    }
    return 0;
}

int ntg_on_upgrade(const uintptr_t ptr, ntg_upgrade_callback callback, void* userData) {
    try {
        getInstance(ptr)->onUpgrade([ptr, callback, userData](const int64_t chatId, const ntgcalls::MediaState state) {
            callback(ptr, chatId, parseCMediaState(state), userData);
        });
    } catch (ntgcalls::NullPointer&) {
        return NTG_ERROR_NULL_POINTER;
    }
    return 0;
}

int ntg_on_connection_change(const uintptr_t ptr, ntg_connection_callback callback, void* userData) {
    try {
        getInstance(ptr)->onConnectionChange([ptr, callback, userData](const int64_t chatId, const ntgcalls::NetworkInfo state) {
            callback(ptr, chatId, parseCNetworkInfo(state), userData);
        });
    } catch (ntgcalls::NullPointer&) {
        return NTG_ERROR_NULL_POINTER;
    }
    return 0;
}

int ntg_on_signaling_data(uintptr_t ptr, ntg_signaling_callback callback, void* userData) {
    try {
        getInstance(ptr)->onSignalingData([ptr, callback, userData](const int64_t userId, const bytes::binary& data) {
            uint8_t* buffer;
            int bufferSize;
            copyAndReturn(data, &buffer, &bufferSize);
            callback(ptr, userId, buffer, bufferSize, userData);
            delete[] buffer;
        });
    } catch (ntgcalls::NullPointer&) {
        return NTG_ERROR_NULL_POINTER;
    }
    return 0;
}

int ntg_on_frames(uintptr_t ptr, ntg_frame_callback callback, void* userData) {
    try {
        getInstance(ptr)->onFrames([ptr, callback, userData](const int64_t chatId, const ntgcalls::StreamManager::Mode mode, const ntgcalls::StreamManager::Device device, const std::vector<wrtc::Frame>& frames) {
            auto* buffer = new ntg_frame_struct[frames.size()];
            for (int i = 0; i < frames.size(); i++) {
                ntg_frame_struct frame{};
                frame.ssrc = frames[i].ssrc;
                copyAndReturn(frames[i].data, &frame.data, &frame.sizeData);
                frame.frameData = parseCFrameData(frames[i].frameData);
                buffer[i] = frame;
            }
            callback(ptr, chatId, parseCStreamMode(mode), parseCStreamDevice(device), buffer, frames.size(), userData);
            for (int i = 0; i < frames.size(); i++) {
                delete[] buffer[i].data;
            }
        });
    } catch (ntgcalls::NullPointer&) {
        return NTG_ERROR_NULL_POINTER;
    }
    return 0;
}

int ntg_on_remote_source_change(uintptr_t ptr, ntg_remote_source_callback callback, void* userData) {
    try {
        getInstance(ptr)->onRemoteSourceChange([ptr, callback, userData](const int64_t chatId, const ntgcalls::RemoteSource state) {
            callback(ptr, chatId, {
                state.ssrc,
                parseCStatus(state.state),
                parseCStreamDevice(state.device),
            }, userData);
        });
    } catch (ntgcalls::NullPointer&) {
        return NTG_ERROR_NULL_POINTER;
    }
    return 0;
}

int ntg_on_request_broadcast_timestamp(uintptr_t ptr, ntg_broadcast_timestamp_callback callback, void* userData) {
    try {
        getInstance(ptr)->onRequestBroadcastTimestamp([ptr, callback, userData](const int64_t chatId) {
            callback(ptr, chatId, userData);
        });
    } catch (ntgcalls::NullPointer&) {
        return NTG_ERROR_NULL_POINTER;
    }
    return 0;
}

int ntg_on_request_broadcast_part(uintptr_t ptr, ntg_broadcast_part_callback callback, void* userData) {
    try {
        getInstance(ptr)->onRequestBroadcastPart([ptr, callback, userData](const int64_t chatId, const wrtc::SegmentPartRequest& partRequest) {
            callback(ptr, chatId, {
                partRequest.segmentId,
                partRequest.partId,
                partRequest.limit,
                partRequest.timestamp,
                partRequest.qualityUpdate,
                partRequest.channelId,
                parseCSegmentQuality(partRequest.quality)
            }, userData);
        });
    } catch (ntgcalls::NullPointer&) {
        return NTG_ERROR_NULL_POINTER;
    }
    return 0;
}

int ntg_get_version(char** buffer) {
    return copyAndReturn(NTG_VERSION, buffer);
}

void ntg_register_logger(ntg_log_message_callback callback) {
    ntgcalls::LogSink::registerLogger([callback](const ntgcalls::LogSink::LogMessage &message) {
        char *fileName, *messageContent;
        copyAndReturn(message.file, &fileName);
        copyAndReturn(message.message, &messageContent);
        callback({
            parseCLevel(message.level),
            parseCSource(message.source),
            fileName,
            message.line,
            messageContent
        });
        delete[] fileName;
        delete[] messageContent;
    });
}