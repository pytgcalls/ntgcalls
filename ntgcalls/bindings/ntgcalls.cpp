//
// Created by Laky64 on 29/08/2023.
//

#include "../ntgcalls.hpp"
#include "ntgcalls.h"

#include "ntgcalls/exceptions.hpp"

std::map<uint32_t, std::shared_ptr<ntgcalls::NTgCalls>> clients;
uint32_t uidGenerator;

int copyAndReturn(const std::vector<std::byte>& b, uint8_t *buffer, const int size) {
    if (!buffer)
        return static_cast<int>(b.size());

    if (size < static_cast<int>(b.size()))
        return NTG_ERR_TOO_SMALL;
    const auto *bufferTemp = reinterpret_cast<const uint8_t*>(b.data());
#ifndef IS_MACOS
    std::copy_n(bufferTemp, b.size(), buffer);
#else
    std::copy(bufferTemp, bufferTemp + b.size(), buffer);
#endif
    return static_cast<int>(b.size());
}

bytes::vector copyAndReturn(const uint8_t *buffer, const int size) {
    bytes::vector b(size);
    const auto *bufferTemp = reinterpret_cast<const std::byte*>(buffer);
#ifndef IS_MACOS
    std::copy_n(bufferTemp, size, b.begin());
#else
    std::copy(bufferTemp, bufferTemp + size, b.begin());
#endif
    return b;
}

int copyAndReturn(std::string s, char *buffer, const int size) {
    if (!buffer)
        return static_cast<int>(s.size() + 1);

    if (size < static_cast<int>(s.size() + 1))
        return NTG_ERR_TOO_SMALL;

#ifndef IS_MACOS
    std::ranges::copy(s, buffer);
#else
    std::copy(s.begin(), s.end(), buffer);
#endif

    buffer[s.size()] = '\0';
    return static_cast<int>(s.size() + 1);
}

#define PREPARE_ASYNC(method, ...) \
const auto result = new int(NTG_ASYNC_NOT_READY);\
try {\
safeUID(uid)->method(__VA_ARGS__).then(\

#define PREPARE_ASYNC_END \
);\
} catch (ntgcalls::InvalidUUID&) {\
*result = NTG_INVALID_UID;\
} catch (...) {\
*result = NTG_UNKNOWN_EXCEPTION;\
}\
return std::move(result);

template <typename T>
int copyAndReturn(std::vector<T> b, T *buffer, const int size) {
    if (!buffer)
        return static_cast<int>(b.size());

    if (size < static_cast<int>(b.size()))
        return NTG_ERR_TOO_SMALL;
    std::copy(b.begin(), b.end(), buffer);
    return static_cast<int>(b.size());
}

std::shared_ptr<ntgcalls::NTgCalls> safeUID(const uint32_t uid) {
    if (!clients.contains(uid)) {
        throw ntgcalls::InvalidUUID("UUID" + std::to_string(uid) + " not found");
    }
    return clients[uid];
}

ntgcalls::BaseMediaDescription::InputMode parseInputMode(const ntg_input_mode_enum mode) {
    constexpr auto result = ntgcalls::BaseMediaDescription::InputMode::Unknown;
    if (mode & NTG_FILE) {
        result |= ntgcalls::BaseMediaDescription::InputMode::File;
    }
    if (mode & NTG_SHELL) {
        result |= ntgcalls::BaseMediaDescription::InputMode::Shell;
    }
    if (mode & NTG_FFMPEG) {
        result |= ntgcalls::BaseMediaDescription::InputMode::FFmpeg;
    }
    if (mode & NTG_NO_LATENCY) {
        result |= ntgcalls::BaseMediaDescription::InputMode::NoLatency;
    }
    return result;
}

ntg_media_state_struct parseMediaState(const ntgcalls::MediaState state) {
    return ntg_media_state_struct{
            state.muted,
            state.videoPaused,
            state.videoStopped,
    };
}

ntg_stream_status_enum parseStatus(const ntgcalls::Stream::Status status) {
    switch (status) {
        case ntgcalls::Stream::Playing:
            return NTG_PLAYING;
        case ntgcalls::Stream::Paused:
            return NTG_PAUSED;
        case ntgcalls::Stream::Idling:
            return NTG_IDLING;
    }
    return {};
}

std::vector<wrtc::RTCServer> parseRTCServers(ntg_rtc_server_struct* servers, const int size) {
    std::vector<wrtc::RTCServer> serversCpp;
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
    for (int i = 0; i < size; i++) {
        versionsCpp.emplace_back(versions[i]);
    }
    return versionsCpp;
}

std::pair<char**, int> copyAndReturn(const std::vector<std::string>& versions) {
    auto versionsCpp = new char*[versions.size()];
    for (int i = 0; i < versions.size(); i++) {
        versionsCpp[i] = new char[versions[i].size() + 1];
        copyAndReturn(versions[i], versionsCpp[i], static_cast<int>(versions[i].size() + 1));
    }
    return {versionsCpp, static_cast<int>(versions.size())};
}

ntgcalls::MediaDescription parseMediaDescription(const ntg_media_description_struct& desc) {
    std::optional<ntgcalls::AudioDescription> audio;
    std::optional<ntgcalls::VideoDescription> video;
    if (desc.audio) {
        if (desc.audio->inputMode & (NTG_FILE | NTG_SHELL)) {
            audio = ntgcalls::AudioDescription(
                parseInputMode(desc.audio->inputMode),
                desc.audio->sampleRate,
                desc.audio->bitsPerSample,
                desc.audio->channelCount,
                std::string(desc.audio->input)
            );
        } else {
            throw ntgcalls::FFmpegError("Not supported");
        }
    }
    if (desc.video) {
        if (desc.video->inputMode & (NTG_FILE | NTG_SHELL)) {
            video = ntgcalls::VideoDescription(
                parseInputMode(desc.video->inputMode),
                desc.video->width,
                desc.video->height,
                desc.video->fps,
                std::string(desc.video->input)
            );
        } else {
            throw ntgcalls::FFmpegError("Not supported");
        }
    }
    return {
        audio,
        video
    };
}

uint32_t ntg_init() {
    const uint32_t uid = uidGenerator++;
    clients[uid] = std::make_shared<ntgcalls::NTgCalls>();
    return uid;
}

int ntg_destroy(const uint32_t uid) {
    if (!clients.contains(uid)) {
        return NTG_INVALID_UID;
    }
    clients.erase(clients.find(uid));
    return 0;
}


int* ntg_create_p2p(const uint32_t uid, const int64_t userId, const int32_t g, const uint8_t* p, const int sizeP, const uint8_t* r, const int sizeR, const uint8_t* g_a_hash, const int sizeGAHash, const ntg_media_description_struct desc, uint8_t* buffer, const int size, ntg_async_callback callback) {
    PREPARE_ASYNC(createP2PCall, userId, g, copyAndReturn(p, sizeP), copyAndReturn(r, sizeR), sizeGAHash ? std::optional(copyAndReturn(g_a_hash, sizeGAHash)) : std::nullopt, parseMediaDescription(desc))
    [result, callback, buffer, size] (const bytes::vector& s) {
        *result = copyAndReturn(s, buffer, size);
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::ConnectionError&) {
            *result = NTG_CONNECTION_ALREADY_EXISTS;
        } catch (ntgcalls::FileError&) {
            *result = NTG_FILE_NOT_FOUND;
        } catch (ntgcalls::FFmpegError&) {
            *result = NTG_FFMPEG_NOT_FOUND;
        } catch (ntgcalls::ShellError&) {
            *result = NTG_SHELL_ERROR;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_exchange_keys(const uint32_t uid, const int64_t userId, const uint8_t* p, const int sizeP, const uint8_t* g_a_or_b, const int sizeGAB, const int64_t fingerprint, ntg_auth_params_struct *authParams, ntg_async_callback callback) {
    PREPARE_ASYNC(exchangeKeys, userId, copyAndReturn(p, sizeP), copyAndReturn(g_a_or_b, sizeGAB), fingerprint)
    [result, callback, authParams](const ntgcalls::AuthParams& params) {
        authParams->key_fingerprint = params.key_fingerprint;
        authParams->g_a_or_b = new uint8_t[params.g_a_or_b.size()];
        if (const auto rTemp = copyAndReturn(params.g_a_or_b, authParams->g_a_or_b, authParams->sizeGAOrBSize); rTemp < 0) {
            *result = rTemp;
        } else {
            *result = 0;
            authParams->sizeGAOrBSize = rTemp;
        }
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::ConnectionError&) {
            *result = NTG_CONNECTION_ALREADY_EXISTS;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (ntgcalls::CryptoError&) {
            *result = NTG_CRYPTO_ERROR;
        } catch (ntgcalls::InvalidParams&) {
            *result = NTG_MISSING_FINGERPRINT;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}


int* ntg_connect_p2p(const uint32_t uid, const int64_t userId, ntg_rtc_server_struct* servers, const int serversSize, char** versions, const int versionsSize, ntg_async_callback callback) {
    PREPARE_ASYNC(connectP2P, userId, parseRTCServers(servers, serversSize), copyAndReturn(versions, versionsSize))
    [result, callback] {
        *result = 0;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::ConnectionError&) {
            *result = NTG_CONNECTION_ALREADY_EXISTS;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (ntgcalls::TelegramServerError&) {
            *result = NTG_CONNECTION_FAILED;
        } catch (ntgcalls::CryptoError&) {
            *result = NTG_CRYPTO_ERROR;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_send_signaling_data(const uint32_t uid, const int64_t userId, uint8_t* buffer, const int size, ntg_async_callback callback) {
    PREPARE_ASYNC(sendSignalingData, userId, bytes::binary(buffer, buffer + size))
    [result, callback] {
        *result = 0;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_get_protocol(const uint32_t uid, ntg_protocol_struct* protocol) {
    const auto result = new int(0);
    try {
        const auto [min_layer, max_layer, udp_p2p, udp_reflector, library_versions] = safeUID(uid)->getProtocol();
        protocol->minLayer = min_layer;
        protocol->maxLayer = max_layer;
        protocol->udpP2P = udp_p2p;
        protocol->udpReflector = udp_reflector;
        auto [libraryVersions, libraryVersionsSize] = copyAndReturn(library_versions);
        protocol->libraryVersionsSize = libraryVersionsSize;
        protocol->libraryVersions = libraryVersions;
    } catch (ntgcalls::InvalidUUID&) {
        *result = NTG_INVALID_UID;
    } catch (...) {
        *result = NTG_UNKNOWN_EXCEPTION;
    }
    return result;
}

int* ntg_create(const uint32_t uid, const int64_t chatID, const ntg_media_description_struct desc, char* buffer, const int size, ntg_async_callback callback) {
    PREPARE_ASYNC(createCall, chatID, parseMediaDescription(desc))
    [result, callback, buffer, size](const std::string& s) {
        *result = copyAndReturn(s, buffer, size);
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::ConnectionError&) {
            *result = NTG_CONNECTION_ALREADY_EXISTS;
        } catch (ntgcalls::FileError&) {
            *result = NTG_FILE_NOT_FOUND;
        } catch (ntgcalls::InvalidParams&) {
            *result = NTG_ENCODER_NOT_FOUND;
        } catch (ntgcalls::FFmpegError&) {
            *result = NTG_FFMPEG_NOT_FOUND;
        } catch (ntgcalls::ShellError&) {
            *result = NTG_SHELL_ERROR;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_connect(const uint32_t uid, const int64_t chatID, char* params, ntg_async_callback callback) {
    PREPARE_ASYNC(connect, chatID, std::string(params))
    [result, callback] {
        *result = 0;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::RTMPNeeded&) {
            *result = NTG_RTMP_NEEDED;
        } catch (ntgcalls::InvalidParams&) {
            *result = NTG_INVALID_TRANSPORT;
        } catch (ntgcalls::ConnectionError&) {
            *result = NTG_CONNECTION_FAILED;
        } catch(ntgcalls::TelegramServerError&) {
            *result = NTG_CONNECTION_FAILED;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_change_stream(const uint32_t uid, const int64_t chatID, const ntg_media_description_struct desc, ntg_async_callback callback) {
    PREPARE_ASYNC(changeStream, chatID, parseMediaDescription(desc))
    [result, callback] {
        *result = 0;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::RTMPNeeded&) {
            *result = NTG_RTMP_NEEDED;
        } catch (ntgcalls::InvalidParams&) {
            *result = NTG_INVALID_TRANSPORT;
        } catch (ntgcalls::ConnectionError&) {
            *result = NTG_CONNECTION_FAILED;
        } catch(ntgcalls::TelegramServerError&) {
            *result = NTG_CONNECTION_FAILED;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_pause(const uint32_t uid, const int64_t chatID, ntg_async_callback callback) {
    PREPARE_ASYNC(pause, chatID)
    [result, callback](const bool success) {
        *result = !success;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::InvalidUUID&) {
            *result = NTG_INVALID_UID;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_resume(const uint32_t uid, const int64_t chatID, ntg_async_callback callback) {
    PREPARE_ASYNC(resume, chatID)
    [result, callback](const bool success) {
        *result = !success;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::InvalidUUID&) {
            *result = NTG_INVALID_UID;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_mute(const uint32_t uid, const int64_t chatID, ntg_async_callback callback) {
    PREPARE_ASYNC(mute, chatID)
    [result, callback](const bool success) {
        *result = !success;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::InvalidUUID&) {
            *result = NTG_INVALID_UID;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_unmute(const uint32_t uid, const int64_t chatID, ntg_async_callback callback) {
    PREPARE_ASYNC(unmute, chatID)
    [result, callback](const bool success) {
        *result = !success;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::InvalidUUID&) {
            *result = NTG_INVALID_UID;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_stop(const uint32_t uid, const int64_t chatID, ntg_async_callback callback) {
    PREPARE_ASYNC(stop, chatID)
    [result, callback] {
        *result = 0;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::InvalidUUID&) {
            *result = NTG_INVALID_UID;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_time(const uint32_t uid, const int64_t chatID, int64_t* time, ntg_async_callback callback) {
    PREPARE_ASYNC(time, chatID)
    [result, callback, time](const int64_t t) {
        *time = t;
        *result = 0;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::InvalidUUID&) {
            *result = NTG_INVALID_UID;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_get_state(const uint32_t uid, const int64_t chatID, ntg_media_state_struct* mediaState, ntg_async_callback callback) {
    PREPARE_ASYNC(getState, chatID)
    [result, callback, mediaState](const ntgcalls::MediaState state) {
        *mediaState = parseMediaState(state);
        *result = 0;
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::InvalidUUID&) {
            *result = NTG_INVALID_UID;
        } catch (ntgcalls::ConnectionNotFound&) {
            *result = NTG_CONNECTION_NOT_FOUND;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_calls(const uint32_t uid, ntg_group_call_struct *buffer, const int size, ntg_async_callback callback) {
    PREPARE_ASYNC(calls)
    [result, callback, buffer, size](const auto callsCpp) {
        std::vector<ntg_group_call_struct> groupCalls;
        for (const auto [fst, snd] : callsCpp) {
            groupCalls.push_back(ntg_group_call_struct{
                fst,
                parseStatus(snd),
            });
        }
        *result = copyAndReturn(groupCalls, buffer, size);
        callback();
    },
    [result, callback](const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (ntgcalls::InvalidUUID&) {
            *result = NTG_INVALID_UID;
        } catch (...) {
            *result = NTG_UNKNOWN_EXCEPTION;
        }
        callback();
    }
    PREPARE_ASYNC_END
}

int* ntg_calls_count(const uint32_t uid, uint64_t* size, ntg_async_callback callback) {
    PREPARE_ASYNC(calls)
    [result, size, callback](const auto callsCpp) {
        *size = callsCpp.size();
        *result = 0;
        callback();
    },
    [result](const std::exception_ptr&) {
        *result = NTG_UNKNOWN_EXCEPTION;
    }
    PREPARE_ASYNC_END
}

int* ntg_cpu_usage(const uint32_t uid, double *buffer, ntg_async_callback callback) {
    PREPARE_ASYNC(cpuUsage)
    [result, callback, buffer](const double usage) {
        *buffer = usage;
        *result = 0;
        callback();
    },
    [result, callback](const std::exception_ptr&) {
        *result = NTG_UNKNOWN_EXCEPTION;
        callback();
    }
    PREPARE_ASYNC_END
}

int ntg_on_stream_end(const uint32_t uid, ntg_stream_callback callback) {
    try {
        safeUID(uid)->onStreamEnd([uid, callback](const int64_t chatId, const ntgcalls::Stream::Type type) {
            callback(uid, chatId, type == ntgcalls::Stream::Type::Audio ? NTG_STREAM_AUDIO : NTG_STREAM_VIDEO);
        });
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
    return 0;
}

int ntg_on_upgrade(const uint32_t uid, ntg_upgrade_callback callback) {
    try {
        safeUID(uid)->onUpgrade([uid, callback](const int64_t chatId, const ntgcalls::MediaState state) {
            callback(uid, chatId, parseMediaState(state));
        });
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    }
    return 0;
}

int ntg_on_disconnect(const uint32_t uid, ntg_disconnect_callback callback) {
    try {
        safeUID(uid)->onDisconnect([uid, callback](const int64_t chatId) {
            callback(uid, chatId);
        });
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    }
    return 0;
}

int ntg_on_signaling_data(uint32_t uid, ntg_signaling_callback callback) {
    try {
        safeUID(uid)->onSignalingData([uid, callback](const int64_t userId, const bytes::binary& data) {
            auto* buffer = new uint8_t[data.size()];
            const auto bufferSize = copyAndReturn(data, buffer, static_cast<int>(data.size()));
            callback(uid, userId, buffer, bufferSize);
        });
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    }
    return 0;
}

int ntg_get_version(char* buffer, const int size) {
    return copyAndReturn(NTG_VERSION, buffer, size);
}