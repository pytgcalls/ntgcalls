//
// Created by Laky64 on 15/09/2024.
//

#pragma once

#include <jni.h>
#include "ntgcalls/ntgcalls.hpp"

ntgcalls::NTgCalls* getInstance(JNIEnv *env, jobject obj);

ntgcalls::AudioDescription parseAudioDescription(JNIEnv *env, jobject audioDescription);

ntgcalls::VideoDescription parseVideoDescription(JNIEnv *env, jobject videoDescription);

ntgcalls::MediaDescription parseMediaDescription(JNIEnv *env, jobject mediaDescription);

ntgcalls::BaseMediaDescription::InputMode parseInputMode(JNIEnv *env, jint inputMode);

void throwJavaException(JNIEnv *env, std::string name, std::string message);