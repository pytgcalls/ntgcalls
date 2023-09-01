//
// Created by Laky64 on 29/08/2023.
//

#include <iostream>
#include "../ntgcalls.hpp"
#include "ntgcalls.h"

std::map<uint32_t, std::shared_ptr<ntgcalls::NTgCalls>> clients;
uint64_t uidGenerator;

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

void DestroyNTgCalls(uint32_t uid, int8_t *errorCode) {
    if (clients.find(uid) == clients.end()) {
        *errorCode = INVALID_UID;
        return;
    }
    clients.erase(clients.find(uid));
}

const char* CreateCall(uint32_t uid, int64_t chatID, MediaDescription desc, int8_t *errorCode) {
    try {
        const char *res;
        res = safeUID(uid)->createCall(chatID, parseMediaDescription(desc)).c_str();
        return res;
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    } catch (ntgcalls::ConnectionError) {
        *errorCode = CONNECTION_ALREADY_EXISTS;
    } catch (ntgcalls::FileError) {
        *errorCode = FILE_NOT_FOUND;
    } catch (ntgcalls::InvalidParams) {
        *errorCode = ENCODER_NOT_FOUND;
    } catch (ntgcalls::FFmpegError) {
        *errorCode = FFMPEG_NOT_FOUND;
    } catch (...) {
        *errorCode = UNKNOWN_EXCEPTION;
    }
    return nullptr;
}

void ConnectCall(uint32_t uid, int64_t chatID, char* params, int8_t *errorCode) {
    try {
        safeUID(uid)->connect(chatID, std::string(params));
    } catch (ntgcalls::RTMPNeeded) {
        *errorCode = RTMP_NEEDED;
    } catch (ntgcalls::InvalidParams) {
        *errorCode = INVALID_TRANSPORT;
    } catch (ntgcalls::ConnectionError) {
        *errorCode = CONNECTION_FAILED;
    } catch (ntgcalls::ConnectionNotFound) {
        *errorCode = CONNECTION_NOT_FOUND;
    } catch (...) {
        *errorCode = UNKNOWN_EXCEPTION;
    }
}

void ChangeStream(uint32_t uid, int64_t chatID, MediaDescription desc, int8_t *errorCode) {
    try {
        safeUID(uid)->changeStream(chatID, parseMediaDescription(desc));
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    } catch (ntgcalls::FileError) {
        *errorCode = FILE_NOT_FOUND;
    } catch (ntgcalls::InvalidParams) {
        *errorCode = ENCODER_NOT_FOUND;
    } catch (ntgcalls::FFmpegError) {
        *errorCode = FFMPEG_NOT_FOUND;
    } catch (ntgcalls::ConnectionNotFound) {
        *errorCode = CONNECTION_NOT_FOUND;
    } catch (...) {
        *errorCode = UNKNOWN_EXCEPTION;
    }
}

bool Pause(uint32_t uid, int64_t chatID, int8_t *errorCode) {
    try {
        return safeUID(uid)->pause(chatID);
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound) {
        *errorCode = CONNECTION_NOT_FOUND;
    }
    return false;
}

bool Resume(uint32_t uid, int64_t chatID, int8_t *errorCode) {
    try {
        return safeUID(uid)->resume(chatID);
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound) {
        *errorCode = CONNECTION_NOT_FOUND;
    }
    return false;
}

bool Mute(uint32_t uid, int64_t chatID, int8_t *errorCode) {
    try {
        return safeUID(uid)->mute(chatID);
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound) {
        *errorCode = CONNECTION_NOT_FOUND;
    }
    return false;
}

bool UnMute(uint32_t uid, int64_t chatID, int8_t *errorCode) {
    try {
        return safeUID(uid)->unmute(chatID);
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound) {
        *errorCode = CONNECTION_NOT_FOUND;
    }
    return false;
}

void Stop(uint32_t uid, int64_t chatID, int8_t *errorCode) {
    try {
        safeUID(uid)->stop(chatID);
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound) {
        *errorCode = CONNECTION_NOT_FOUND;
    }
}

uint64_t Time(uint32_t uid, int64_t chatID, int8_t *errorCode) {
    try {
        return safeUID(uid)->time(chatID);
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    } catch (ntgcalls::ConnectionNotFound) {
        *errorCode = CONNECTION_NOT_FOUND;
    }
    return 0;
}

void OnStreamEnd(uint32_t uid, StreamEndCallback callback, int8_t *errorCode) {
    try {
        safeUID(uid)->onStreamEnd([uid, callback](int64_t chatId, ntgcalls::Stream::Type type) {
            callback(uid, chatId, type == ntgcalls::Stream::Type::Audio ? StreamType::Audio:StreamType::Video);
        });
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    } catch (...) {
        *errorCode = UNKNOWN_EXCEPTION;
    }
}

void OnUpgrade(uint32_t uid, UpgradeCallback callback, int8_t *errorCode) {
    try {
        safeUID(uid)->onUpgrade([uid, callback](int64_t chatId, ntgcalls::MediaState state) {
            callback(uid, chatId, MediaState{
                state.muted,
                state.videoPaused,
                state.videoStopped,
            });
        });
    } catch (ntgcalls::InvalidUUID) {
        *errorCode = INVALID_UID;
    }
}