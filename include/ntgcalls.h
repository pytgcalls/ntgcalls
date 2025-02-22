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
    NTG_ERROR_CONNECTION_NOT_FOUND = -101,
    NTG_ERROR_CRYPTO = -102,
    NTG_ERROR_SIGNALING = -104,
    NTG_ERROR_SIGNALING_UNSUPPORTED = -105,
    NTG_ERROR_INVALID_PARAMS = -106,

    // STREAM
    NTG_ERROR_FILE = -200,
    NTG_ERROR_FFMPEG = -202,
    NTG_ERROR_SHELL = -203,
    NTG_ERROR_MEDIA_DEVICE = -204,

    // WebRTC
    NTG_ERROR_RTMP_NEEDED = -300,
    NTG_ERROR_PARSE_TRANSPORT = -301,
    NTG_ERROR_CONNECTION = -302,
    NTG_ERROR_TELEGRAM_SERVER = -303,
    NTG_ERROR_WEBRTC = -304,
    NTG_ERROR_PARSE_SDP = -305,

    // Others
    NTG_ERROR_UNKNOWN = -1,
    NTG_ERROR_NULL_POINTER = -2,
    NTG_ERROR_TOO_SMALL = -3,
    NTG_ERROR_ASYNC_NOT_READY = -4
} ntg_error_code_enum;

typedef enum {
    NTG_FILE = 1 << 0,
    NTG_SHELL = 1 << 1,
    NTG_FFMPEG = 1 << 2,
    NTG_DEVICE = 1 << 3,
    NTG_DESKTOP = 1 << 4,
    NTG_EXTERNAL = 1 << 5
} ntg_media_source_enum;

typedef enum {
    NTG_STREAM_MICROPHONE,
    NTG_STREAM_SPEAKER,
    NTG_STREAM_CAMERA,
    NTG_STREAM_SCREEN
} ntg_stream_device_enum;

typedef enum {
    NTG_STREAM_CAPTURE,
    NTG_STREAM_PLAYBACK,
} ntg_stream_mode_enum;

typedef enum {
    NTG_STREAM_AUDIO,
    NTG_STREAM_VIDEO
} ntg_stream_type_enum;

typedef enum {
    NTG_ACTIVE,
    NTG_PAUSED,
    NTG_IDLING
} ntg_stream_status_enum;

typedef enum {
    NTG_STATE_CONNECTING,
    NTG_STATE_CONNECTED,
    NTG_STATE_TIMEOUT,
    NTG_STATE_FAILED,
    NTG_STATE_CLOSED,
} ntg_connection_state_enum;

typedef enum{
    NTG_KIND_NORMAL,
    NTG_KIND_PRESENTATION
} ntg_connection_kind_enum;

typedef struct {
    ntg_connection_kind_enum kind;
    ntg_connection_state_enum state;
} ntg_call_network_state_struct;

typedef struct {
    ntg_media_source_enum mediaSource;
    char* input;
    uint32_t sampleRate;
    uint8_t channelCount;
} ntg_audio_description_struct;

typedef struct {
    ntg_media_source_enum mediaSource;
    char* input;
    int16_t width, height;
    uint8_t fps;
} ntg_video_description_struct;

typedef struct {
    uint8_t* g_a_or_b;
    int sizeGAB;
    int64_t key_fingerprint;
} ntg_auth_params_struct;

typedef struct {
    ntg_audio_description_struct* microphone;
    ntg_audio_description_struct* speaker;
    ntg_video_description_struct* camera;
    ntg_video_description_struct* screen;
} ntg_media_description_struct;

typedef struct {
    int64_t chatId;
    ntg_stream_status_enum capture;
    ntg_stream_status_enum playback;
} ntg_call_struct;

typedef struct {
    bool muted;
    bool videoPaused;
    bool videoStopped;
    bool presentationPaused;
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

typedef struct {
    int32_t g;
    const uint8_t* p;
    int sizeP;
    const uint8_t* random;
    int sizeRandom;
} ntg_dh_config_struct;

typedef struct {
    int64_t absoluteCaptureTimestampMs;
    uint16_t width, height;
    uint16_t rotation;
} ntg_frame_data_struct;

typedef enum {
    NTG_REMOTE_ACTIVE,
    NTG_REMOTE_SUSPENDED,
    NTG_REMOTE_INACTIVE
} ntg_remote_source_state_enum;

typedef struct {
    uint32_t ssrc;
    ntg_remote_source_state_enum state;
    ntg_stream_device_enum device;
} ntg_remote_source_struct;

typedef struct {
    char* semantics;
    uint32_t* ssrcs;
    int sizeSsrcs;
} ntg_ssrc_group_struct;

typedef void (*ntg_async_callback)(void*);

typedef struct {
    void* userData;
    int* errorCode;
    char** errorMessage;
    ntg_async_callback promise;
} ntg_async_struct;

typedef struct {
    char* name;
    char* metadata;
} ntg_device_info_struct;

typedef struct {
    ntg_device_info_struct* microphone;
    int sizeMicrophone;
    ntg_device_info_struct* speaker;
    int sizeSpeaker;
    ntg_device_info_struct* camera;
    int sizeCamera;
    ntg_device_info_struct* screen;
    int sizeScreen;
} ntg_media_devices_struct;

typedef struct {
    int64_t ssrc;
    uint8_t* data;
    int sizeData;
    ntg_frame_data_struct frameData;
} ntg_frame_struct;

typedef void (*ntg_stream_callback)(uintptr_t, int64_t, ntg_stream_type_enum, ntg_stream_device_enum, void*);

typedef void (*ntg_upgrade_callback)(uintptr_t, int64_t, ntg_media_state_struct, void*);

typedef void (*ntg_connection_callback)(uintptr_t, int64_t, ntg_call_network_state_struct, void*);

typedef void (*ntg_signaling_callback)(uintptr_t, int64_t, uint8_t*, int, void*);

typedef void (*ntg_frame_callback)(uintptr_t, int64_t, ntg_stream_mode_enum, ntg_stream_device_enum, ntg_frame_struct*, int, void*);

typedef void (*ntg_remote_source_callback)(uintptr_t, int64_t, ntg_remote_source_struct, void*);

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

NTG_C_EXPORT uintptr_t ntg_init();

NTG_C_EXPORT int ntg_destroy(uintptr_t ptr);

NTG_C_EXPORT int ntg_create_p2p(uintptr_t ptr, int64_t userId, ntg_media_description_struct desc, ntg_async_struct future);

NTG_C_EXPORT int ntg_init_presentation(uintptr_t ptr, int64_t chatId, char* buffer, int size, ntg_async_struct future);

NTG_C_EXPORT int ntg_stop_presentation(uintptr_t ptr, int64_t chatId, ntg_async_struct future);

NTG_C_EXPORT int ntg_add_incoming_video(uintptr_t ptr, int64_t chatId, char* endpoint, ntg_ssrc_group_struct* ssrcGroups, int size, uint32_t* buffer, ntg_async_struct future);

NTG_C_EXPORT int ntg_remove_incoming_video(uintptr_t ptr, int64_t chatId, char* endpoint, ntg_async_struct future);

NTG_C_EXPORT int ntg_init_exchange(uintptr_t ptr, int64_t userId, ntg_dh_config_struct* dhConfig, const uint8_t* g_a_hash, int sizeGAHash, uint8_t* buffer, int size, ntg_async_struct future);

NTG_C_EXPORT int ntg_exchange_keys(uintptr_t ptr, int64_t userId, const uint8_t* g_a_or_b, int sizeGAB, int64_t fingerprint, ntg_auth_params_struct *buffer, ntg_async_struct future);

NTG_C_EXPORT int ntg_skip_exchange(uintptr_t ptr, int64_t userId, const uint8_t* encryptionKey, int size, bool isOutgoing, ntg_async_struct future);

NTG_C_EXPORT int ntg_connect_p2p(uintptr_t ptr, int64_t userId, ntg_rtc_server_struct* servers, int serversSize, char** versions, int versionsSize, bool p2pAllowed, ntg_async_struct future);

NTG_C_EXPORT int ntg_send_signaling_data(uintptr_t ptr, int64_t userId, uint8_t* buffer, int size, ntg_async_struct future);

NTG_C_EXPORT int ntg_get_protocol(ntg_protocol_struct *buffer);

NTG_C_EXPORT int ntg_create(uintptr_t ptr, int64_t chatID, ntg_media_description_struct desc, char* buffer, int size, ntg_async_struct future);

NTG_C_EXPORT int ntg_connect(uintptr_t ptr, int64_t chatID, char* params, bool isPresentation, ntg_async_struct future);

NTG_C_EXPORT int ntg_set_stream_sources(uintptr_t ptr, int64_t chatID, ntg_stream_mode_enum streamMode, ntg_media_description_struct desc, ntg_async_struct future);

NTG_C_EXPORT int ntg_pause(uintptr_t ptr, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_resume(uintptr_t ptr, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_mute(uintptr_t ptr, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_unmute(uintptr_t ptr, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_stop(uintptr_t ptr, int64_t chatID, ntg_async_struct future);

NTG_C_EXPORT int ntg_time(uintptr_t ptr, int64_t chatID, ntg_stream_mode_enum streamMode, int64_t* time, ntg_async_struct future);

NTG_C_EXPORT int ntg_get_state(uintptr_t ptr, int64_t chatID, ntg_media_state_struct *mediaState, ntg_async_struct future);

NTG_C_EXPORT int ntg_send_external_frame(uintptr_t ptr, int64_t chatID, ntg_stream_device_enum device, uint8_t* frame, int frameSize, ntg_frame_data_struct frameData, ntg_async_struct future);

NTG_C_EXPORT int ntg_get_media_devices(ntg_media_devices_struct *buffer);

NTG_C_EXPORT int ntg_calls(uintptr_t ptr, ntg_call_struct *buffer, uint64_t size, ntg_async_struct future);

NTG_C_EXPORT int ntg_calls_count(uintptr_t ptr, uint64_t* size, ntg_async_struct future);

NTG_C_EXPORT int ntg_on_stream_end(uintptr_t ptr, ntg_stream_callback callback, void* userData);

NTG_C_EXPORT int ntg_on_upgrade(uintptr_t ptr, ntg_upgrade_callback callback, void* userData);

NTG_C_EXPORT int ntg_on_connection_change(uintptr_t ptr, ntg_connection_callback callback, void* userData);

NTG_C_EXPORT int ntg_on_signaling_data(uintptr_t ptr, ntg_signaling_callback callback, void* userData);

NTG_C_EXPORT int ntg_on_frames(uintptr_t ptr, ntg_frame_callback callback, void* userData);

NTG_C_EXPORT int ntg_on_remote_source_change(uintptr_t ptr, ntg_remote_source_callback callback, void* userData);

NTG_C_EXPORT int ntg_get_version(char* buffer, int size);

NTG_C_EXPORT int ntg_cpu_usage(uintptr_t ptr, double *buffer, ntg_async_struct future);

NTG_C_EXPORT int ntg_enable_g_lib_loop(bool enable);

NTG_C_EXPORT int ntg_enable_h264_encoder(bool enable);

#ifdef __cplusplus
}
#endif
