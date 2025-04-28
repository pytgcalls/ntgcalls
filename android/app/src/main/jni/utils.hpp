//
// Created by Laky64 on 15/09/2024.
//

#pragma once

#include <jni.h>
#include <ntgcalls/ntgcalls.hpp>
#include <ntgcalls/exceptions.hpp>
#include <sdk/android/native_api/jni/scoped_java_ref.h>
#include <ntgcalls/devices/java_audio_device_module.hpp>
#include <ntgcalls/devices/java_video_capturer_module.hpp>

struct JavaCallback {
    jobject callback;
    jmethodID methodId;
};

struct InstanceCallbacks {
    std::optional<JavaCallback> onUpgradeCallback;
    std::optional<JavaCallback> onStreamEndCallback;
    std::optional<JavaCallback> onConnectionChangeCallback;
    std::optional<JavaCallback> onSignalingDataCallback;
    std::optional<JavaCallback> onFramesCallback;
    std::optional<JavaCallback> onRemoteSourceChangeCallback;
    std::optional<JavaCallback> onRequestBroadcastPartCallback;
    std::optional<JavaCallback> onRequestBroadcastTimestampCallback;
};

ntgcalls::NTgCalls* getInstance(JNIEnv *env, jobject obj);

ntgcalls::JavaAudioDeviceModule* getInstanceAudioCapture(JNIEnv *env, jobject obj);

ntgcalls::JavaVideoCapturerModule* getInstanceVideoCapture(JNIEnv *env, jobject obj);

jlong getInstancePtr(JNIEnv *env, jobject obj);

ntgcalls::AudioDescription parseAudioDescription(JNIEnv *env, jobject audioDescription);

ntgcalls::VideoDescription parseVideoDescription(JNIEnv *env, jobject videoDescription);

ntgcalls::MediaDescription parseMediaDescription(JNIEnv *env, jobject mediaDescription);

ntgcalls::DhConfig parseDhConfig(JNIEnv *env, jobject dhConfig);

std::string parseString(JNIEnv *env, jstring string);

webrtc::ScopedJavaLocalRef<jstring> parseJString(JNIEnv *env, const std::string& string);

bytes::vector parseByteArray(JNIEnv *env, jbyteArray byteArray);

webrtc::ScopedJavaLocalRef<jbyteArray> parseJByteArray(JNIEnv *env, const bytes::vector& byteArray);

bytes::unique_binary parseUniqueBinary(JNIEnv *env, jbyteArray byteArray);

bytes::binary parseBinary(JNIEnv *env, jbyteArray byteArray);

webrtc::ScopedJavaLocalRef<jbyteArray> parseJBinary(JNIEnv *env, const bytes::binary& binary);

webrtc::ScopedJavaLocalRef<jobject> parseAuthParams(JNIEnv *env, const ntgcalls::AuthParams& authParams);

std::vector<std::string> parseStringList(JNIEnv *env, jobject list);

webrtc::ScopedJavaLocalRef<jobject> parseJStringList(JNIEnv *env, const std::vector<std::string>& list);

ntgcalls::RTCServer parseRTCServer(JNIEnv *env, jobject rtcServer);

std::vector<ntgcalls::RTCServer> parseRTCServerList(JNIEnv *env, jobject list);

ntgcalls::BaseMediaDescription::MediaSource parseMediaSource(JNIEnv *env, jobject inputMode);

webrtc::ScopedJavaLocalRef<jobject> parseJMediaState(JNIEnv *env, ntgcalls::MediaState mediaState);

webrtc::ScopedJavaLocalRef<jobject> parseJProtocol(JNIEnv *env, const ntgcalls::Protocol& protocol);

webrtc::ScopedJavaLocalRef<jobject> parseJStreamType(JNIEnv *env, ntgcalls::StreamManager::Type type);

webrtc::ScopedJavaLocalRef<jobject> parseJNetworkInfo(JNIEnv *env, ntgcalls::NetworkInfo state);

webrtc::ScopedJavaLocalRef<jobject> parseJConnectionState(JNIEnv *env, ntgcalls::NetworkInfo::ConnectionState state);

webrtc::ScopedJavaLocalRef<jobject> parseJNetworkInfoKind(JNIEnv *env, ntgcalls::NetworkInfo::Kind kind);

webrtc::ScopedJavaLocalRef<jobject> parseJStreamStatus(JNIEnv *env, ntgcalls::StreamManager::Status status);

webrtc::ScopedJavaLocalRef<jobject> parseJCallInfo(JNIEnv *env, const ntgcalls::StreamManager::CallInfo& status);

webrtc::ScopedJavaLocalRef<jobject> parseJCallInfoMap(JNIEnv *env, const std::map<int64_t, ntgcalls::StreamManager::CallInfo>& calls);

webrtc::ScopedJavaLocalRef<jobject> parseJMediaDevices(JNIEnv *env, const ntgcalls::MediaDevices& devices);

webrtc::ScopedJavaLocalRef<jobject> parseJDeviceInfoList(JNIEnv *env, const std::vector<ntgcalls::DeviceInfo>& devices);

webrtc::ScopedJavaLocalRef<jobject> parseJDeviceInfo(JNIEnv *env, const ntgcalls::DeviceInfo& device);

ntgcalls::StreamManager::Mode parseStreamMode(JNIEnv *env, jobject mode);

webrtc::ScopedJavaLocalRef<jobject> parseJStreamMode(JNIEnv *env, ntgcalls::StreamManager::Mode mode);

ntgcalls::StreamManager::Device parseDevice(JNIEnv *env, jobject device);

webrtc::ScopedJavaLocalRef<jobject> parseJDevice(JNIEnv *env, ntgcalls::StreamManager::Device device);

wrtc::FrameData parseFrameData(JNIEnv *env, jobject frameData);

webrtc::ScopedJavaLocalRef<jobject> parseJFrameData(JNIEnv *env, const wrtc::FrameData& frameData);

webrtc::ScopedJavaLocalRef<jobject> parseJRemoteSource(JNIEnv *env, const ntgcalls::RemoteSource& source);

wrtc::SsrcGroup parseSsrcGroup(JNIEnv *env, jobject ssrcGroup);

std::vector<wrtc::SsrcGroup> parseSsrcGroupList(JNIEnv *env, jobject list);

webrtc::ScopedJavaLocalRef<jobject> parseJFrame(JNIEnv *env, const wrtc::Frame& frame);

webrtc::ScopedJavaLocalRef<jobject> parseJFrames(JNIEnv *env, const std::vector<wrtc::Frame>& frame);

webrtc::ScopedJavaLocalRef<jobject> parseJSegmentPartRequest(JNIEnv *env, const wrtc::SegmentPartRequest& request);

webrtc::ScopedJavaLocalRef<jobject> parseJMediaSegmentQuality(JNIEnv *env, const wrtc::MediaSegment::Quality& quality);

wrtc::MediaSegment::Part::Status parseSegmentPartStatus(JNIEnv *env, jobject status);

webrtc::ScopedJavaLocalRef<jobject> parseJConnectionMode(JNIEnv *env, wrtc::NetworkInterface::Mode mode);

void throwJavaException(JNIEnv *env, std::string name, const std::string& message);

#define TRANSLATE_NTG_EXCEPTION(x) \
catch (const ntgcalls::x& e) {  \
throwJavaException(env, #x, e.what()); \
}

#define TRANSLATE_WRTC_EXCEPTION(x) \
catch (const wrtc::x& e) {  \
throwJavaException(env, #x, e.what()); \
}

#define HANDLE_EXCEPTIONS \
TRANSLATE_NTG_EXCEPTION(InvalidParams) \
TRANSLATE_NTG_EXCEPTION(ShellError) \
TRANSLATE_NTG_EXCEPTION(FFmpegError) \
TRANSLATE_NTG_EXCEPTION(ConnectionError) \
TRANSLATE_NTG_EXCEPTION(ConnectionNotFound) \
TRANSLATE_NTG_EXCEPTION(TelegramServerError) \
TRANSLATE_NTG_EXCEPTION(CryptoError) \
TRANSLATE_NTG_EXCEPTION(SignalingError) \
TRANSLATE_NTG_EXCEPTION(SignalingUnsupported) \
TRANSLATE_NTG_EXCEPTION(MediaDeviceError) \
TRANSLATE_NTG_EXCEPTION(RTCConnectionNeeded) \
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
JNIEXPORT void JNICALL Java_io_github_pytgcalls_NTgCalls_##name(JNIEnv *env, jobject thiz, jobject callback) { \
std::lock_guard lock(callbacksMutex); \
auto instancePtr = getInstancePtr(env, thiz); \
if (auto checkCallback = callbacksInstances[instancePtr].method##Callback) { \
env->DeleteGlobalRef(checkCallback->callback); \
} \
auto callbackClass = env->GetObjectClass(callback); \
if (callbackClass == nullptr) {                     \
return; \
} \
callbacksInstances[instancePtr].method##Callback = JavaCallback{ \
env->NewGlobalRef(callback), \
env->GetMethodID(callbackClass, #method, data_class) \
}; \
env->DeleteLocalRef(callbackClass);\
}

#define CAPTURE_JAVA_EXCEPTION if (env->ExceptionCheck()) { \
env->ExceptionDescribe(); \
env->ExceptionClear(); \
}