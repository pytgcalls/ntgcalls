//
// Created by Laky64 on 29/08/2023.
//

#include "../ntgcalls.hpp"
#include "ntgcalls.h"

#include "ntgcalls/exceptions.hpp"

std::map<uint32_t, std::shared_ptr<ntgcalls::NTgCalls>> clients;
uint32_t uidGenerator;

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

template <typename T> int copyAndReturn(std::vector<T> b, T *buffer, const int size) {
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
    switch (mode) {
        case NTG_FILE:
            return ntgcalls::BaseMediaDescription::InputMode::File;
        case NTG_SHELL:
            return ntgcalls::BaseMediaDescription::InputMode::Shell;
        case NTG_FFMPEG:
            return ntgcalls::BaseMediaDescription::InputMode::FFmpeg;
    }
    return {};
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

ntgcalls::MediaDescription parseMediaDescription(const ntg_media_description_struct& desc) {
    std::optional<ntgcalls::AudioDescription> audio;
    std::optional<ntgcalls::VideoDescription> video;
    if (desc.audio) {
        switch (desc.audio->inputMode) {
            case NTG_FILE:
            case NTG_SHELL:
                audio = ntgcalls::AudioDescription(
                    parseInputMode(desc.audio->inputMode),
                    desc.audio->sampleRate,
                    desc.audio->bitsPerSample,
                    desc.audio->channelCount,
                    std::string(desc.audio->input)
                );
                break;
            case NTG_FFMPEG:
                throw ntgcalls::FFmpegError("Not supported");
        }
    }
    if (desc.video) {
        switch (desc.video->inputMode) {
            case NTG_FILE:
            case NTG_SHELL:
                video = ntgcalls::VideoDescription(
                    parseInputMode(desc.video->inputMode),
                    desc.video->width,
                    desc.video->height,
                    desc.video->fps,
                    std::string(desc.video->input)
                );
                break;
            case NTG_FFMPEG:
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

int ntg_get_params(const uint32_t uid, const int64_t chatID, const ntg_media_description_struct desc, char* buffer, const int size) {
    try {
        return copyAndReturn(safeUID(uid)->createCall(chatID, parseMediaDescription(desc)), buffer, size);
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::ConnectionError&) {
        return NTG_CONNECTION_ALREADY_EXISTS;
    } catch (ntgcalls::FileError&) {
        return NTG_FILE_NOT_FOUND;
    } catch (ntgcalls::InvalidParams&) {
        return NTG_ENCODER_NOT_FOUND;
    } catch (ntgcalls::FFmpegError&) {
        return NTG_FFMPEG_NOT_FOUND;
    } catch (ntgcalls::ShellError&) {
        return NTG_SHELL_ERROR;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
}

int ntg_connect(const uint32_t uid, const int64_t chatID, char* params) {
    try {
        safeUID(uid)->connect(chatID, std::string(params));
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::RTMPNeeded&) {
        return NTG_RTMP_NEEDED;
    } catch (ntgcalls::InvalidParams&) {
        return NTG_INVALID_TRANSPORT;
    } catch (ntgcalls::ConnectionError&) {
        return NTG_CONNECTION_FAILED;
    } catch (ntgcalls::ConnectionNotFound&) {
        return NTG_CONNECTION_NOT_FOUND;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
    return 0;
}

int ntg_change_stream(const uint32_t uid, const int64_t chatID, const ntg_media_description_struct desc) {
    try {
        safeUID(uid)->changeStream(chatID, parseMediaDescription(desc));
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::FileError&) {
        return NTG_FILE_NOT_FOUND;
    } catch (ntgcalls::InvalidParams&) {
        return NTG_ENCODER_NOT_FOUND;
    } catch (ntgcalls::FFmpegError&) {
        return NTG_FFMPEG_NOT_FOUND;
    } catch (ntgcalls::ShellError&) {
        return NTG_SHELL_ERROR;
    } catch (ntgcalls::ConnectionNotFound&) {
        return NTG_CONNECTION_NOT_FOUND;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
    return 0;
}

int ntg_pause(const uint32_t uid, const int64_t chatID) {
    try {
        return !safeUID(uid)->pause(chatID);
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound&) {
        return NTG_CONNECTION_NOT_FOUND;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
}

int ntg_resume(const uint32_t uid, const int64_t chatID) {
    try {
        return !safeUID(uid)->resume(chatID);
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound&) {
        return NTG_CONNECTION_NOT_FOUND;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
}

int ntg_mute(const uint32_t uid, const int64_t chatID) {
    try {
        return !safeUID(uid)->mute(chatID);
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound&) {
        return NTG_CONNECTION_NOT_FOUND;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
}

int ntg_unmute(const uint32_t uid, const int64_t chatID) {
    try {
        return !safeUID(uid)->unmute(chatID);
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound&) {
        return NTG_CONNECTION_NOT_FOUND;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
}

int ntg_stop(const uint32_t uid, const int64_t chatID) {
    try {
        safeUID(uid)->stop(chatID);
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound&) {
        return NTG_CONNECTION_NOT_FOUND;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
    return 0;
}

int64_t ntg_time(const uint32_t uid, const int64_t chatID) {
    try {
        return static_cast<int64_t>(safeUID(uid)->time(chatID));
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound&) {
        return NTG_CONNECTION_NOT_FOUND;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
}

auto ntg_get_state(const uint32_t uid, const int64_t chatID, ntg_media_state_struct* mediaState) -> int
{
    try {
        *mediaState = parseMediaState(safeUID(uid)->getState(chatID));
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound&) {
        return NTG_CONNECTION_NOT_FOUND;
    } catch (...) {
        return NTG_UNKNOWN_EXCEPTION;
    }
    return 0;
}

int ntg_calls(const uint32_t uid, ntg_group_call_struct *buffer, const int size) {
    try {
        const auto callsCpp = safeUID(uid)->calls();
        std::vector<ntg_group_call_struct> groupCalls;
        for (const auto [fst, snd] : callsCpp) {
            groupCalls.push_back(ntg_group_call_struct{
                fst,
                parseStatus(snd),
            });
        }
        return copyAndReturn(groupCalls, buffer, size);
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    }
}

int ntg_calls_count(const uint32_t uid) {
    try {
        return static_cast<int>(safeUID(uid)->calls().size());
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    }
}

int ntg_on_stream_end(uint32_t uid, ntg_stream_callback callback) {
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

int ntg_on_upgrade(uint32_t uid, ntg_upgrade_callback callback) {
    try {
        safeUID(uid)->onUpgrade([uid, callback](const int64_t chatId, const ntgcalls::MediaState state) {
            callback(uid, chatId, parseMediaState(state));
        });
    } catch (ntgcalls::InvalidUUID&) {
        return NTG_INVALID_UID;
    }
    return 0;
}

int ntg_get_version(char* buffer, const int size) {
    return copyAndReturn(NTG_VERSION, buffer, size);
}