//
// Created by Laky64 on 15/09/2024.
//

#pragma once

#include <jni.h>
#include <ntgcalls/ntgcalls.hpp>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/devices/java_audio_device_module.hpp>

struct JavaCallback {
    jobject callback;
    jmethodID methodId;
};

struct InstanceCallbacks {
    std::optional<JavaCallback> onUpgradeCallback;
    std::optional<JavaCallback> onStreamEndCallback;
    std::optional<JavaCallback> onConnectionChangeCallback;
    std::optional<JavaCallback> onSignalingDataCallback;
};

ntgcalls::NTgCalls* getInstance(JNIEnv *env, jobject obj);

ntgcalls::JavaAudioDeviceModule* getInstanceAudioCapture(JNIEnv *env, jobject obj);

jlong getInstancePtr(JNIEnv *env, jobject obj);

ntgcalls::AudioDescription parseAudioDescription(JNIEnv *env, jobject audioDescription);

ntgcalls::VideoDescription parseVideoDescription(JNIEnv *env, jobject videoDescription);

ntgcalls::MediaDescription parseMediaDescription(JNIEnv *env, jobject mediaDescription);

jobject parseMediaDevices(JNIEnv *env, const ntgcalls::MediaDevices& devices);

jobject parseDeviceInfoList(JNIEnv *env, const std::vector<ntgcalls::DeviceInfo>& devices);

jobject parseDeviceInfo(JNIEnv *env, const ntgcalls::DeviceInfo& device);

ntgcalls::DhConfig parseDhConfig(JNIEnv *env, jobject dhConfig);

std::string parseString(JNIEnv *env, jstring string);

jstring parseJString(JNIEnv *env, const std::string& string);

bytes::vector parseByteArray(JNIEnv *env, jbyteArray byteArray);

jbyteArray parseJByteArray(JNIEnv *env, const bytes::vector& byteArray);

bytes::unique_binary parseUniqueBinary(JNIEnv *env, jbyteArray byteArray);

bytes::binary parseBinary(JNIEnv *env, jbyteArray byteArray);

jbyteArray parseJBinary(JNIEnv *env, const bytes::binary& binary);

jobject parseAuthParams(JNIEnv *env, const ntgcalls::AuthParams& authParams);

std::vector<std::string> parseStringList(JNIEnv *env, jobject list);

jobject parseJStringList(JNIEnv *env, const std::vector<std::string>& list);

ntgcalls::RTCServer parseRTCServer(JNIEnv *env, jobject rtcServer);

std::vector<ntgcalls::RTCServer> parseRTCServerList(JNIEnv *env, jobject list);

ntgcalls::BaseMediaDescription::InputMode parseInputMode(jint inputMode);

jobject parseMediaState(JNIEnv *env, ntgcalls::MediaState mediaState);

jobject parseProtocol(JNIEnv *env, const ntgcalls::Protocol& protocol);

jobject parseStreamType(JNIEnv *env, ntgcalls::StreamManager::Type type);

jobject parseConnectionState(JNIEnv *env, ntgcalls::CallInterface::ConnectionState state);

jobject parseStreamStatus(JNIEnv *env, ntgcalls::StreamManager::Status status);

jobject parseMediaStatus(JNIEnv *env, const ntgcalls::StreamManager::MediaStatus& status);

jobject parseMediaStatusMap(JNIEnv *env, const std::map<int64_t, ntgcalls::StreamManager::MediaStatus>& calls);

ntgcalls::StreamManager::Mode parseStreamMode(JNIEnv *env, jobject mode);

jobject parseJDevice(JNIEnv *env, ntgcalls::StreamManager::Device device);

void throwJavaException(JNIEnv *env, std::string name, const std::string& message);

#define TRANSLATE_NTG_EXCEPTION(x) \
catch (const ntgcalls::x& e) {  \
throwJavaException(env, #x, e.what()); \
}

#define HANDLE_EXCEPTIONS \
TRANSLATE_NTG_EXCEPTION(InvalidParams) \
TRANSLATE_NTG_EXCEPTION(ShellError) \
TRANSLATE_NTG_EXCEPTION(FFmpegError) \
TRANSLATE_NTG_EXCEPTION(ConnectionError) \
TRANSLATE_NTG_EXCEPTION(ConnectionNotFound) \
TRANSLATE_NTG_EXCEPTION(TelegramServerError) \
TRANSLATE_NTG_EXCEPTION(RTMPNeeded) \
TRANSLATE_NTG_EXCEPTION(CryptoError) \
TRANSLATE_NTG_EXCEPTION(SignalingError) \
TRANSLATE_NTG_EXCEPTION(SignalingUnsupported)\
TRANSLATE_NTG_EXCEPTION(MediaDeviceError) \
catch (const ntgcalls::FileError& e) { \
throwJavaException(env, "FileNotFoundException", e.what()); \
} \
catch (const std::exception& e) { \
throwJavaException(env, "RuntimeException", e.what()); \
} \
catch (...) { \
throwJavaException(env, "RuntimeException", "Unknown error"); \
}

#define REGISTER_CALLBACK(name, method, data_class) \
extern "C" \
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_##name(JNIEnv *env, jobject thiz, jobject callback) { \
auto instancePtr = getInstancePtr(env, thiz); \
if (auto checkCallback = callbacksInstances[instancePtr].method##Callback) { \
env->DeleteGlobalRef(checkCallback->callback); \
} \
auto callbackClass = env->GetObjectClass(callback); \
callbacksInstances[instancePtr].method##Callback = JavaCallback{ \
env->NewGlobalRef(callback), \
env->GetMethodID(callbackClass, #method, data_class) \
}; \
env->DeleteLocalRef(callbackClass);\
}