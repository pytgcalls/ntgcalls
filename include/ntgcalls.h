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

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <stdbool.h>

// EXCEPTIONS CODES
typedef enum {
    // NTgCalls
    NTG_CONNECTION_ALREADY_EXISTS = -100,
    NTG_CONNECTION_NOT_FOUND = -101,
    NTG_CRYPTO_ERROR = -102,
    NTG_MISSING_FINGERPRINT = -103,
    NTG_SIGNALING_ERROR = -104,
    NTG_SIGNALING_UNSUPPORTED = -105,

    // STREAM
    NTG_FILE_NOT_FOUND = -200,
    NTG_ENCODER_NOT_FOUND = -201,
    NTG_FFMPEG_NOT_FOUND = -202,
    NTG_SHELL_ERROR = -203,

    // WebRTC
    NTG_RTMP_NEEDED = -300,
    NTG_INVALID_TRANSPORT = -301,
    NTG_CONNECTION_FAILED = -302,

    // Others
    NTG_UNKNOWN_EXCEPTION = -1,
    NTG_INVALID_UID = -2,
    NTG_ERR_TOO_SMALL = -3,
    NTG_ASYNC_NOT_READY = -4
} ntg_error_code_enum;

typedef enum {
    NTG_FILE = 1 << 0,
    NTG_SHELL = 1 << 1,
    NTG_FFMPEG = 1 << 2,
    NTG_NO_LATENCY = 1 << 3,
} ntg_input_mode_enum;

typedef enum {
    NTG_STREAM_AUDIO,
    NTG_STREAM_VIDEO
} ntg_stream_type_enum;

typedef enum {
    NTG_PLAYING,
    NTG_PAUSED,
    NTG_IDLING
} ntg_stream_status_enum;

typedef struct {
    ntg_input_mode_enum inputMode;
    char* input;
    uint32_t sampleRate;
    uint8_t bitsPerSample, channelCount;
} ntg_audio_description_struct;

typedef struct {
    ntg_input_mode_enum inputMode;
    char* input;
    uint16_t width, height;
    uint8_t fps;
} ntg_video_description_struct;

typedef struct {
    uint8_t* g_a_or_b;
    int sizeGAB;
    int64_t key_fingerprint;
} ntg_auth_params_struct;

typedef struct {
    ntg_audio_description_struct* audio;
    ntg_video_description_struct* video;
} ntg_media_description_struct;

typedef struct {
    int64_t chatId;
    ntg_stream_status_enum status;
} ntg_group_call_struct;

typedef struct {
    bool muted;
    bool videoPaused;
    bool videoStopped;
} ntg_media_state_struct;

typedef struct {
    uint64_t id;
    char* ipv4;
    char* ipv6;
    char* username;
    char* password;
    uint16_t port;
    bool turn;
    bool stun;
    bool tcp;
    uint8_t* peerTag;
    int peerTagSize;
} ntg_rtc_server_struct;

typedef struct {
    int32_t minLayer;
    int32_t maxLayer;
    bool udpP2P;
    bool udpReflector;
    char** libraryVersions;
    int libraryVersionsSize;
} ntg_protocol_struct;

typedef void (*ntg_async_callback)(void*);

typedef struct {
    void* userData;
    int* errorCode;
    ntg_async_callback promise;
} ntg_async_struct;

typedef void (*ntg_stream_callback)(uint32_t, int64_t, ntg_stream_type_enum, void*);

typedef void (*ntg_upgrade_callback)(uint32_t, int64_t, ntg_media_state_struct, void*);

typedef void (*ntg_disconnect_callback)(uint32_t, int64_t, void*);

typedef void (*ntg_signaling_callback)(uint32_t, int64_t, uint8_t*, int, void*);

typedef enum {
    NTG_LOG_DEBUG = 1 << 0,
    NTG_LOG_INFO = 1 << 1,
    NTG_LOG_WARNING = 1 << 2,
    NTG_LOG_ERROR = 1 << 3,
    NTG_LOG_UNKNOWN = -1,
} ntg_log_level_enum;

typedef enum {
    NTG_LOG_WEBRTC = 1 << 0,
    NTG_LOG_SELF = 1 << 1
} ntg_log_source_enum;

typedef struct {
    ntg_log_level_enum level;
    ntg_log_source_enum source;
    char* file;
    uint32_t line;
    char* message;
} ntg_log_message_struct;

typedef void (*ntg_log_message_callback)(ntg_log_message_struct);

NTG_C_EXPORT void ntg_register_logger(ntg_log_message_callback callback);

NTG_C_EXPORT uint32_t ntg_init();

NTG_C_EXPORT int ntg_destroy(uint32_t uid);

NTG_C_EXPORT int ntg_create_p2p(uint32_t uid, int64_t userId, int32_t g, const uint8_t* p, int sizeP, const uint8_t* r, int sizeR, const uint8_t* g_a_hash, int sizeGAHash, ntg_media_description_struct desc, uint8_t* buffer, int size, ntg_async_struct future);

NTG_C_EXPORT int ntg_exchange_keys(uint32_t uid, int64_t userId, const uint8_t* g_a_or_b, int sizeGAB, int64_t fingerprint, ntg_auth_params_struct *authParams, ntg_async_struct future);

NTG_C_EXPORT int ntg_connect_p2p(uint32_t uid, int64_t userId, ntg_rtc_server_struct* servers, int serversSize, char** versions, int versionsSize, bool p2pAllowed, ntg_async_struct future);

NTG_C_EXPORT int ntg_send_signaling_data(uint32_t uid, int64_t userId, uint8_t* buffer, int size, ntg_async_struct future);

NTG_C_EXPORT int ntg_get_protocol(uint32_t uid, ntg_protocol_struct *protocol);

NTG_C_EXPORT int ntg_create(uint32_t uid, int64_t chatID, ntg_media_description_struct desc, char* buffer, int size, ntg_async_struct future);

NTG_C_EXPORT int ntg_connect(uint32_t uid, int64_t chatID, char* params, ntg_async_struct future);

NTG_C_EXPORT int ntg_change_stream(uint32_t uid, int64_t chatID, ntg_media_description_struct desc, ntg_async_struct future);

NTG_C_EXPORT int ntg_pause(uint32_t uid, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_resume(uint32_t uid, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_mute(uint32_t uid, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_unmute(uint32_t uid, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_stop(uint32_t uid, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_time(uint32_t uid, int64_t chatID, int64_t* time, ntg_async_struct future);

NTG_C_EXPORT int ntg_get_state(uint32_t uid, int64_t chatID, ntg_media_state_struct *mediaState, ntg_async_struct future);

NTG_C_EXPORT int ntg_calls(uint32_t uid, ntg_group_call_struct *buffer, uint64_t size, ntg_async_struct future);

NTG_C_EXPORT int ntg_calls_count(uint32_t uid, uint64_t* size, ntg_async_struct future);

NTG_C_EXPORT int ntg_on_stream_end(uint32_t uid, ntg_stream_callback callback, void* userData);

NTG_C_EXPORT int ntg_on_upgrade(uint32_t uid, ntg_upgrade_callback callback, void* userData);

NTG_C_EXPORT int ntg_on_disconnect(uint32_t uid, ntg_disconnect_callback callback, void* userData);

NTG_C_EXPORT int ntg_on_signaling_data(uint32_t uid, ntg_signaling_callback callback, void* userData);

NTG_C_EXPORT int ntg_get_version(char* buffer, int size);

NTG_C_EXPORT int ntg_cpu_usage(uint32_t uid, double *buffer, ntg_async_struct future);

#ifdef __cplusplus
}
#endif
