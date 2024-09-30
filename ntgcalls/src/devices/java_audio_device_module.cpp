//
// Created by Laky64 on 30/09/24.
//

#include <ntgcalls/devices/java_audio_device_module.hpp>

#ifdef IS_ANDROID
#include <wrtc/utils/java_context.hpp>

namespace ntgcalls {
    JavaAudioDeviceModule::JavaAudioDeviceModule(const AudioDescription* desc, const bool isCapture, BaseSink* sink):
        BaseDeviceModule(desc, isCapture),
        BaseReader(sink)
    {
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        const auto audioRecordClass = env->FindClass("org/pytgcalls/ntgcalls/devices/JavaAudioDeviceModule");
        // ReSharper disable once CppLocalVariableMayBeConst
        auto localJavaModule = env->NewObject(
            audioRecordClass,
            env->GetMethodID(audioRecordClass, "<init>", "(ZIIJ)V"),
            isCapture,
            desc->sampleRate,
            desc->channelCount,
            reinterpret_cast<jlong>(this)
        );
        javaModule = env->NewGlobalRef(localJavaModule);
        env->DeleteLocalRef(audioRecordClass);
        env->DeleteLocalRef(localJavaModule);
    }

    JavaAudioDeviceModule::~JavaAudioDeviceModule() {
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        jclass javaModuleClass = env->GetObjectClass(javaModule);
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass, "release", "()V"));
        env->DeleteGlobalRef(javaModule);
        env->DeleteLocalRef(javaModuleClass);
    }

    void JavaAudioDeviceModule::onRecordedData(bytes::unique_binary data) const {
        dataCallback(std::move(data));
    }

    void JavaAudioDeviceModule::open() {
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        jclass javaModuleClass = env->GetObjectClass(javaModule);
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass, "open", "()V"));
        env->DeleteLocalRef(javaModuleClass);
    }

    bool JavaAudioDeviceModule::isSupported() {
        return android_get_device_api_level() >= __ANDROID_API_J_MR2__;
    }
} // ntgcalls

#endif