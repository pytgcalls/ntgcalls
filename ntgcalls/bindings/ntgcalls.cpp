//
// Created by Laky64 on 29/08/2023.
//

#include <iostream>
#include "../ntgcalls.hpp"
#include "ntgcalls.h"

std::map<uint32_t, std::shared_ptr<ntgcalls::NTgCalls>> clients;
std::unordered_set<uint32_t> generatedNumbers;

int CreateUniqueRandomId() {
    auto uid = rtc::CreateRandomId();
    if (generatedNumbers.find(uid) == generatedNumbers.end()) {
        generatedNumbers.insert(uid);
        return uid;
    }
    return CreateUniqueRandomId();
}

std::shared_ptr<ntgcalls::NTgCalls> safeUID(uint32_t uid) {
    if (generatedNumbers.find(uid) == generatedNumbers.end()) {
        throw ntgcalls::InvalidUUID("UUID" + std::to_string(uid) + " not found");
    }
    return clients[uid];
}

ntgcalls::MediaDescription parseMediaDescription(MediaDescription& desc) {
    std::optional<ntgcalls::AudioDescription> audio;
    std::optional<ntgcalls::VideoDescription> video;
    if (desc.audio) {
        std::optional<ntgcalls::FFmpegOptions> options;
        if (desc.audio->options) {
            options = ntgcalls::FFmpegOptions(
                desc.audio->options->streamId
            );
        }
        audio = ntgcalls::AudioDescription(
            desc.audio->sampleRate,
            desc.audio->bitsPerSample,
            desc.audio->channelCount,
            std::string(desc.audio->path),
            options
        );
    }
    if (desc.video) {
        std::optional<ntgcalls::FFmpegOptions> options = std::nullopt;
        if (desc.video->options) {
            options = ntgcalls::FFmpegOptions(
                desc.video->options->streamId
            );
        }
        video = ntgcalls::VideoDescription(
            desc.video->width,
            desc.video->height,
            desc.video->fps,
            std::string(desc.video->path),
            options
        );
    }
    return ntgcalls::MediaDescription(
        std::string(desc.encoder),
        audio,
        video
    );
}

uint32_t CreateNTgCalls() {
    int uid = CreateUniqueRandomId();
    clients[uid] = std::make_shared<ntgcalls::NTgCalls>();
    return uid;
}

void DestroyNTgCalls(uint32_t uid, int8_t *errorCode) {
    if (generatedNumbers.find(uid) == generatedNumbers.end()) {
        *errorCode = INVALID_UID;
        return;
    }
    generatedNumbers.erase(generatedNumbers.find(uid));
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