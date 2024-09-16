//
// Created by Laky64 on 15/09/2024.
//

#pragma once

#include <jni.h>
#include "ntgcalls/ntgcalls.hpp"
#include <ntgcalls/exceptions.hpp>

ntgcalls::NTgCalls* getInstance(JNIEnv *env, jobject obj);

ntgcalls::AudioDescription parseAudioDescription(JNIEnv *env, jobject audioDescription);

ntgcalls::VideoDescription parseVideoDescription(JNIEnv *env, jobject videoDescription);

ntgcalls::MediaDescription parseMediaDescription(JNIEnv *env, jobject mediaDescription);

ntgcalls::DhConfig parseDhConfig(JNIEnv *env, jobject dhConfig);

std::string parseString(JNIEnv *env, jstring string);

jstring parseJString(JNIEnv *env, const std::string& string);

bytes::vector parseByteArray(JNIEnv *env, jbyteArray byteArray);

jbyteArray parseJByteArray(JNIEnv *env, const bytes::vector& byteArray);

bytes::binary parseBinary(JNIEnv *env, jbyteArray byteArray);

jobject parseAuthParams(JNIEnv *env, const ntgcalls::AuthParams& authParams);

std::vector<std::string> parseStringList(JNIEnv *env, jobject list);

jobject parseJStringList(JNIEnv *env, const std::vector<std::string>& list);

ntgcalls::RTCServer parseRTCServer(JNIEnv *env, jobject rtcServer);

std::vector<ntgcalls::RTCServer> parseRTCServerList(JNIEnv *env, jobject list);

ntgcalls::BaseMediaDescription::InputMode parseInputMode(jint inputMode);

jobject parseMediaState(JNIEnv *env, ntgcalls::MediaState mediaState);

jobject parseProtocol(JNIEnv *env, const ntgcalls::Protocol& protocol);

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
TRANSLATE_NTG_EXCEPTION(SignalingUnsupported) \
catch (const ntgcalls::FileError& e) { \
throwJavaException(env, "FileNotFoundException", e.what()); \
} \
catch (const std::exception& e) { \
throwJavaException(env, "RuntimeException", e.what()); \
} \
catch (...) { \
throwJavaException(env, "RuntimeException", "Unknown error"); \
}