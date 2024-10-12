//
// Created by Laky64 on 30/09/24.
//

#include <ntgcalls/devices/java_audio_device_module.hpp>

#ifdef IS_ANDROID
#include <wrtc/utils/java_context.hpp>

namespace ntgcalls {
    JavaAudioDeviceModule::JavaAudioDeviceModule(const AudioDescription* desc, const bool isCapture, BaseSink* sink):
        BaseIO(sink),
        BaseDeviceModule(desc, isCapture),
        BaseReader(sink),
        AudioMixer(sink)
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
        std::lock_guard queueLock(queueMutex);
        running = false;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        jclass javaModuleClass = env->GetObjectClass(javaModule);
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass, "release", "()V"));
        env->DeleteGlobalRef(javaModule);
        env->DeleteLocalRef(javaModuleClass);
    }

    void JavaAudioDeviceModule::onData(bytes::unique_binary data) {
        std::lock_guard lock(queueMutex);
        queue.push(std::move(data));
    }

    void JavaAudioDeviceModule::onRecordedData(bytes::unique_binary data) const {
        dataCallback(std::move(data));
    }

    void JavaAudioDeviceModule::getPlaybackData() {
        auto frameSize = sink->frameSize();
        if (queue.empty()) return;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        jclass javaModuleClass = env->GetObjectClass(javaModule);
        // ReSharper disable once CppLocalVariableMayBeConst
        jfieldID byteBufferFieldID = env->GetFieldID(javaModuleClass, "byteBuffer", "Ljava/nio/ByteBuffer;");
         // ReSharper disable once CppLocalVariableMayBeConst
        jobject byteBufferObject = env->GetObjectField(javaModule, byteBufferFieldID);

        auto* buffer = static_cast<uint8_t*>(env->GetDirectBufferAddress(byteBufferObject));
        if (buffer == nullptr) {
            env->DeleteLocalRef(byteBufferObject);
            env->DeleteLocalRef(javaModuleClass);
            return;
        }
        std::lock_guard lock(queueMutex);
        if (!queue.empty()) {
            memcpy(buffer, queue.front().get(), frameSize);
            queue.pop();
        } else {
            memset(buffer, 0, frameSize);
        }
        env->DeleteLocalRef(byteBufferObject);
        env->DeleteLocalRef(javaModuleClass);
    }

    void JavaAudioDeviceModule::open() {
        if (running) return;
        running = true;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        jclass javaModuleClass = env->GetObjectClass(javaModule);
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass, "open", "()V"));
        env->DeleteLocalRef(javaModuleClass);
    }

    bool JavaAudioDeviceModule::isSupported() {
        return android_get_device_api_level() >= __ANDROID_API_L__;
    }
} // ntgcalls

#endif