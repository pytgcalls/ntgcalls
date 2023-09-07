//
// Created by Laky64 on 29/08/2023.
//

#include <iostream>
#include "../ntgcalls.hpp"
#include "ntgcalls.h"

std::map<uint32_t, std::shared_ptr<ntgcalls::NTgCalls>> clients;
uint64_t uidGenerator;

int copyAndReturn(std::string s, char *buffer, int size) {
    if (!buffer)
        return int(s.size() + 1);

    if (size < int(s.size() + 1))
        return ERR_TOO_SMALL;

    std::copy(s.begin(), s.end(), buffer);
    buffer[s.size()] = '\0';
    return int(s.size() + 1);
}

template <typename T> int copyAndReturn(std::vector<T> b, T *buffer, int size) {
    if (!buffer)
        return int(b.size());

    if (size < int(b.size()))
        return ERR_TOO_SMALL;
    std::copy(b.begin(), b.end(), buffer);
    return int(b.size());
}

std::shared_ptr<ntgcalls::NTgCalls> safeUID(uint32_t uid) {
    if (clients.find(uid) == clients.end()) {
        throw ntgcalls::InvalidUUID("UUID" + std::to_string(uid) + " not found");
    }
    return clients[uid];
}

ntgcalls::BaseMediaDescription::InputMode parseInputMode(InputMode mode) {
    switch (mode) {
        case InputMode::File:
            return ntgcalls::BaseMediaDescription::InputMode::File;
        case InputMode::Shell:
            return ntgcalls::BaseMediaDescription::InputMode::Shell;
        case InputMode::FFmpeg:
            return ntgcalls::BaseMediaDescription::InputMode::FFmpeg;
    }
}

StreamStatus parseStatus(ntgcalls::Stream::Status status) {
    switch (status) {
        case ntgcalls::Stream::Playing:
            return StreamStatus::Playing;
        case ntgcalls::Stream::Paused:
            return StreamStatus::Paused;
        case ntgcalls::Stream::Idling:
            return StreamStatus::Idling;
    }
}

ntgcalls::MediaDescription parseMediaDescription(MediaDescription& desc) {
    std::optional<ntgcalls::AudioDescription> audio;
    std::optional<ntgcalls::VideoDescription> video;
    if (desc.audio) {
        switch (desc.audio->inputMode) {
            case InputMode::File:
            case InputMode::Shell:
                audio = ntgcalls::AudioDescription(
                    parseInputMode(desc.audio->inputMode),
                    desc.audio->sampleRate,
                    desc.audio->bitsPerSample,
                    desc.audio->channelCount,
                    std::string(desc.audio->input)
                );
                break;
            case InputMode::FFmpeg:
                throw ntgcalls::FFmpegError("Not supported");
        }
    }
    if (desc.video) {
        switch (desc.audio->inputMode) {
            case InputMode::File:
            case InputMode::Shell:
                video = ntgcalls::VideoDescription(
                    parseInputMode(desc.audio->inputMode),
                    desc.video->width,
                    desc.video->height,
                    desc.video->fps,
                    std::string(desc.video->input)
                );
                break;
            case InputMode::FFmpeg:
                throw ntgcalls::FFmpegError("Not supported");
        }
    }
    return ntgcalls::MediaDescription(
        audio,
        video
    );
}

uint32_t CreateNTgCalls() {
    int uid = uidGenerator++;
    clients[uid] = std::make_shared<ntgcalls::NTgCalls>();
    return uid;
}

int DestroyNTgCalls(uint32_t uid) {
    if (clients.find(uid) == clients.end()) {
        return INVALID_UID;
    }
    clients.erase(clients.find(uid));
    return 0;
}

int CreateCall(uint32_t uid, int64_t chatID, MediaDescription desc, char* buffer, int size) {
    try {
        return copyAndReturn(safeUID(uid)->createCall(chatID, parseMediaDescription(desc)), buffer, size);
    } catch (ntgcalls::InvalidUUID) {
        return INVALID_UID;
    } catch (ntgcalls::ConnectionError) {
        return CONNECTION_ALREADY_EXISTS;
    } catch (ntgcalls::FileError) {
        return FILE_NOT_FOUND;
    } catch (ntgcalls::InvalidParams) {
        return ENCODER_NOT_FOUND;
    } catch (ntgcalls::FFmpegError) {
        return FFMPEG_NOT_FOUND;
    } catch (ntgcalls::ShellError) {
        return SHELL_ERROR;
    } catch (...) {
        return UNKNOWN_EXCEPTION;
    }
}

int ConnectCall(uint32_t uid, int64_t chatID, char* params) {
    try {
        safeUID(uid)->connect(chatID, std::string(params));
    } catch (ntgcalls::RTMPNeeded) {
        return RTMP_NEEDED;
    } catch (ntgcalls::InvalidParams) {
        return INVALID_TRANSPORT;
    } catch (ntgcalls::ConnectionError) {
        return CONNECTION_FAILED;
    } catch (ntgcalls::ConnectionNotFound) {
        return CONNECTION_NOT_FOUND;
    } catch (...) {
        return UNKNOWN_EXCEPTION;
    }
    return 0;
}

int ChangeStream(uint32_t uid, int64_t chatID, MediaDescription desc) {
    try {
        safeUID(uid)->changeStream(chatID, parseMediaDescription(desc));
    } catch (ntgcalls::InvalidUUID) {
        return INVALID_UID;
    } catch (ntgcalls::FileError) {
        return FILE_NOT_FOUND;
    } catch (ntgcalls::InvalidParams) {
        return ENCODER_NOT_FOUND;
    } catch (ntgcalls::FFmpegError) {
        return FFMPEG_NOT_FOUND;
    } catch (ntgcalls::ShellError) {
        return SHELL_ERROR;
    } catch (ntgcalls::ConnectionNotFound) {
        return CONNECTION_NOT_FOUND;
    } catch (...) {
        return UNKNOWN_EXCEPTION;
    }
    return 0;
}

bool Pause(uint32_t uid, int64_t chatID) {
    try {
        return safeUID(uid)->pause(chatID);
    } catch (...) {}
    return false;
}

bool Resume(uint32_t uid, int64_t chatID) {
    try {
        return safeUID(uid)->resume(chatID);
    } catch (...) {}
    return false;
}

bool Mute(uint32_t uid, int64_t chatID) {
    try {
        return safeUID(uid)->mute(chatID);
    } catch (...) {}
    return false;
}

bool UnMute(uint32_t uid, int64_t chatID) {
    try {
        return safeUID(uid)->unmute(chatID);
    } catch (...) {}
    return false;
}

int Stop(uint32_t uid, int64_t chatID) {
    try {
        safeUID(uid)->stop(chatID);
    } catch (ntgcalls::InvalidUUID) {
        return INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound) {
        return CONNECTION_NOT_FOUND;
    }
    return 0;
}

uint64_t Time(uint32_t uid, int64_t chatID) {
    try {
        return safeUID(uid)->time(chatID);
    } catch (...) {
        return 0;
    }
}

int Calls(uint32_t uid, GroupCall *buffer, int size) {
    try {
        auto callsCpp = safeUID(uid)->calls();
        std::vector<GroupCall> groupCalls;
        for (auto call : callsCpp) {
            groupCalls.push_back(GroupCall{
                call.first,
                parseStatus(call.second),
            });
        }
        return copyAndReturn(groupCalls, buffer, size);
    } catch (ntgcalls::InvalidUUID) {
        return INVALID_UID;
    }
}

int CallsCount(uint32_t uid) {
    try {
        return safeUID(uid)->calls().size();
    } catch (ntgcalls::InvalidUUID) {
        return INVALID_UID;
    }
}

int OnStreamEnd(uint32_t uid, StreamEndCallback callback) {
    try {
        safeUID(uid)->onStreamEnd([uid, callback](int64_t chatId, ntgcalls::Stream::Type type) {
            callback(uid, chatId, type == ntgcalls::Stream::Type::Audio ? StreamType::Audio:StreamType::Video);
        });
    } catch (ntgcalls::InvalidUUID) {
        return INVALID_UID;
    } catch (...) {
        return UNKNOWN_EXCEPTION;
    }
    return 0;
}

int OnUpgrade(uint32_t uid, UpgradeCallback callback) {
    try {
        safeUID(uid)->onUpgrade([uid, callback](int64_t chatId, ntgcalls::MediaState state) {
            callback(uid, chatId, MediaState{
                state.muted,
                state.videoPaused,
                state.videoStopped,
            });
        });
    } catch (ntgcalls::InvalidUUID) {
        return INVALID_UID;
    }
    return 0;
}