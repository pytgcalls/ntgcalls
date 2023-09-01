//
// Created by Laky64 on 29/08/2023.
//

#pragma once

#if defined _WIN32 || defined __CYGWIN__
#ifdef NTG_EXPORTS
#define NTG_C_EXPORT __declspec(dllexport) // building the library
#else
#define NTG_C_EXPORT __declspec(dllimport) // using the library
#endif
#else // not WIN32
#define NTG_C_EXPORT
#endif

#include <stdint.h>
#include <stdbool.h>

// EXCEPTIONS CODES
#define UNKNOWN_EXCEPTION -1;
#define INVALID_UID -2;
#define CONNECTION_ALREADY_EXISTS -3;
#define FILE_NOT_FOUND -4;
#define ENCODER_NOT_FOUND -5;
#define FFMPEG_NOT_FOUND -6;
#define RTMP_NEEDED -7;
#define INVALID_TRANSPORT -8;
#define CONNECTION_FAILED -9;
#define CONNECTION_NOT_FOUND -10;
#define SHELL_ERROR -11;

#ifdef __cplusplus
extern "C" {
#endif

enum InputMode {
    File,
    Shell,
    FFmpeg
};

enum StreamType {
    Audio,
    Video,
};

typedef struct {
    enum InputMode inputMode;
    char* input;
    uint16_t sampleRate;
    uint8_t bitsPerSample, channelCount;
} AudioDescription;

typedef struct {
    enum InputMode inputMode;
    char* input;
    uint16_t width, height;
    uint8_t fps;
} VideoDescription;

typedef struct {
    AudioDescription* audio;
    VideoDescription* video;
} MediaDescription;

typedef struct {
    bool muted;
    bool videoPaused;
    bool videoStopped;
} MediaState;

typedef void (*StreamEndCallback)(uint32_t, int64_t, enum StreamType);

typedef void (*UpgradeCallback)(uint32_t, int64_t, MediaState);

NTG_C_EXPORT uint32_t CreateNTgCalls();

NTG_C_EXPORT void DestroyNTgCalls(uint32_t uid, int8_t *errorCode);

NTG_C_EXPORT const char* CreateCall(uint32_t uid, int64_t chatID, MediaDescription rep, int8_t *errorCode);

NTG_C_EXPORT void ConnectCall(uint32_t uid, int64_t chatID, char* params, int8_t *errorCode);

NTG_C_EXPORT void ChangeStream(uint32_t uid, int64_t chatID, MediaDescription desc, int8_t *errorCode);

NTG_C_EXPORT bool Pause(uint32_t uid, int64_t chatID, int8_t *errorCode);

NTG_C_EXPORT bool Resume(uint32_t uid, int64_t chatID, int8_t *errorCode);

NTG_C_EXPORT bool Mute(uint32_t uid, int64_t chatID, int8_t *errorCode);

NTG_C_EXPORT bool UnMute(uint32_t uid, int64_t chatID, int8_t *errorCode);

NTG_C_EXPORT void Stop(uint32_t uid, int64_t chatID, int8_t *errorCode);

NTG_C_EXPORT uint64_t Time(uint32_t uid, int64_t chatID, int8_t *errorCode);

NTG_C_EXPORT void OnStreamEnd(uint32_t uid, StreamEndCallback callback, int8_t *errorCode);

NTG_C_EXPORT void OnUpgrade(uint32_t uid, UpgradeCallback callback, int8_t *errorCode);

#ifdef __cplusplus
}
#endif
