//
// Created by Laky64 on 30/09/24.
//

#include <ntgcalls/devices/java_audio_device_module.hpp>

#ifdef IS_ANDROID
#include <wrtc/utils/java_context.hpp>
#include <sdk/android/native_api/jni/class_loader.h>
#include <sdk/android/native_api/jni/scoped_java_ref.h>

namespace ntgcalls {
    JavaAudioDeviceModule::JavaAudioDeviceModule(const AudioDescription* desc, const bool isCapture, BaseSink* sink):
        BaseIO(sink),
        BaseDeviceModule(desc, isCapture),
        BaseReader(sink),
        AudioMixer(sink)
    {
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        const webrtc::ScopedJavaLocalRef<jclass> audioRecordClass = webrtc::GetClass(env, "io/github/pytgcalls/devices/JavaAudioDeviceModule");
        webrtc::ScopedJavaLocalRef localJavaModule{env, env->NewObject(
            audioRecordClass.obj(),
            env->GetMethodID(audioRecordClass.obj(), "<init>", "(ZIIJ)V"),
            isCapture,
            desc->sampleRate,
            desc->channelCount,
            reinterpret_cast<jlong>(this)
        )};
        javaModule = env->NewGlobalRef(localJavaModule.Release());
    }

    JavaAudioDeviceModule::~JavaAudioDeviceModule() {
        std::lock_guard queueLock(queueMutex);
        running = false;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        const webrtc::ScopedJavaLocalRef javaModuleClass(env, env->GetObjectClass(javaModule));
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass.obj(), "release", "()V"));
        env->DeleteGlobalRef(javaModule);
    }

    void JavaAudioDeviceModule::onData(bytes::unique_binary data) {
        std::lock_guard lock(queueMutex);
        queue.push(std::move(data));
    }

    void JavaAudioDeviceModule::onRecordedData(bytes::unique_binary data) const {
        dataCallback(std::move(data), {});
    }

    void JavaAudioDeviceModule::getPlaybackData() {
        auto frameSize = sink->frameSize();
        if (queue.empty()) return;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        const webrtc::ScopedJavaLocalRef javaModuleClass{env, env->GetObjectClass(javaModule)};
        // ReSharper disable once CppLocalVariableMayBeConst
        jfieldID byteBufferFieldID = env->GetFieldID(javaModuleClass.obj(), "byteBuffer", "Ljava/nio/ByteBuffer;");
        const webrtc::ScopedJavaLocalRef byteBufferObject{env, env->GetObjectField(javaModule, byteBufferFieldID)};

        auto* buffer = static_cast<uint8_t*>(env->GetDirectBufferAddress(byteBufferObject.obj()));
        if (buffer == nullptr) {
            return;
        }
        std::lock_guard lock(queueMutex);
        if (!queue.empty()) {
            memcpy(buffer, queue.front().get(), frameSize);
            queue.pop();
        } else {
            memset(buffer, 0, frameSize);
        }
    }

    void JavaAudioDeviceModule::open() {
        if (running) return;
        running = true;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        const webrtc::ScopedJavaLocalRef javaModuleClass(env, env->GetObjectClass(javaModule));
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass.obj(), "open", "()V"));
    }

    bool JavaAudioDeviceModule::isSupported() {
        return android_get_device_api_level() >= __ANDROID_API_L__;
    }
} // ntgcalls

#endif