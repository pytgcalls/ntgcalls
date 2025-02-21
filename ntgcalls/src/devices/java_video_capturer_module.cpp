//
// Created by Laky64 on 19/10/24.
//

#ifdef IS_ANDROID

#include <libyuv.h>
#include <ntgcalls/exceptions.hpp>
#include <wrtc/utils/java_context.hpp>
#include <sdk/android/native_api/jni/class_loader.h>
#include <sdk/android/native_api/jni/scoped_java_ref.h>
#include <ntgcalls/devices/java_video_capturer_module.hpp>

namespace ntgcalls {
    JavaVideoCapturerModule::JavaVideoCapturerModule(const bool isScreencast, const VideoDescription& desc, BaseSink* sink): BaseIO(sink), BaseReader(sink), desc(desc) {
        std::string deviceName;
        try {
            auto sourceMetadata = json::parse(desc.input);
            deviceName = sourceMetadata["id"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        if (deviceName == "screen" && !isScreencast) {
            throw MediaDeviceError("Wrong device type");
        }
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        const webrtc::ScopedJavaLocalRef<jclass> videoCapturerClass = webrtc::GetClass(env, "io/github/pytgcalls/devices/JavaVideoCapturerModule");
        webrtc::ScopedJavaLocalRef localJavaModule{env, env->NewObject(
            videoCapturerClass.obj(),
            env->GetMethodID(videoCapturerClass.obj(), "<init>", "(ZLjava/lang/String;IIIJ)V"),
            isScreencast,
            env->NewStringUTF(deviceName.c_str()),
            desc.width,
            desc.height,
            desc.fps,
            reinterpret_cast<jlong>(this)
        )};
        javaModule = env->NewGlobalRef(localJavaModule.Release());
    }

    JavaVideoCapturerModule::~JavaVideoCapturerModule() {
        running = false;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        const webrtc::ScopedJavaLocalRef javaModuleClass(env, env->GetObjectClass(javaModule));
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass.obj(), "release", "()V"));
        env->DeleteGlobalRef(javaModule);
    }

    bool JavaVideoCapturerModule::IsSupported(const bool isScreencast) {
        if (isScreencast) {
            return android_get_device_api_level() >= __ANDROID_API_L__;
        }
        return android_get_device_api_level() >= __ANDROID_API_J_MR2__;
    }

    std::vector<DeviceInfo> JavaVideoCapturerModule::getDevices() {
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        const webrtc::ScopedJavaLocalRef<jclass> videoCapturerClass = webrtc::GetClass(env, "io/github/pytgcalls/devices/JavaVideoCapturerModule");
        // ReSharper disable once CppLocalVariableMayBeConst
        jmethodID getDevicesMethod = env->GetStaticMethodID(videoCapturerClass.obj(), "getDevices", "()Ljava/util/List;");
        const webrtc::ScopedJavaLocalRef deviceList(env, env->CallStaticObjectMethod(videoCapturerClass.obj(), getDevicesMethod));
        const webrtc::ScopedJavaLocalRef<jclass> listClass = webrtc::GetClass(env, "java/util/List");
        // ReSharper disable once CppLocalVariableMayBeConst
        jmethodID listSizeMethod = env->GetMethodID(listClass.obj(), "size", "()I");
        // ReSharper disable once CppLocalVariableMayBeConst
        jmethodID listGetMethod = env->GetMethodID(listClass.obj(), "get", "(I)Ljava/lang/Object;");
        const jint listSize = env->CallIntMethod(deviceList.obj(), listSizeMethod);

        std::vector<DeviceInfo> devices;
        for (jint i = 0; i < listSize; i++) {
            webrtc::ScopedJavaLocalRef deviceInfoObj(env, env->CallObjectMethod(deviceList.obj(), listGetMethod, i));
            webrtc::ScopedJavaLocalRef deviceInfoClass(env, env->GetObjectClass(deviceInfoObj.obj()));
            // ReSharper disable once CppLocalVariableMayBeConst
            jfieldID nameFieldID = env->GetFieldID(deviceInfoClass.obj(), "name", "Ljava/lang/String;");
            // ReSharper disable once CppLocalVariableMayBeConst
            jfieldID metadataFieldID = env->GetFieldID(deviceInfoClass.obj(), "metadata", "Ljava/lang/String;");
            webrtc::ScopedJavaLocalRef nameObj(env, reinterpret_cast<jstring>(env->GetObjectField(deviceInfoObj.obj(), nameFieldID)));
            webrtc::ScopedJavaLocalRef metadataObj(env, reinterpret_cast<jstring>(env->GetObjectField(deviceInfoObj.obj(), metadataFieldID)));
            const auto name = env->GetStringUTFChars(nameObj.obj(), nullptr);
            const auto metadata = env->GetStringUTFChars(metadataObj.obj(), nullptr);
            devices.emplace_back(std::string(name), std::string(metadata));
            env->ReleaseStringUTFChars(nameObj.obj(), name);
            env->ReleaseStringUTFChars(metadataObj.obj(), metadata);
        }
        return devices;
    }

    void JavaVideoCapturerModule::onCapturerStopped() const {
        (void) eofCallback();
    }

    void JavaVideoCapturerModule::onFrame(const webrtc::VideoFrame& frame) {
        const auto yScaledSize = desc.width * desc.height;
        const auto uvScaledSize = yScaledSize / 4;
        auto yuv = bytes::make_unique_binary(yScaledSize + uvScaledSize * 2);
        const auto buffer = frame.video_frame_buffer()->ToI420();

        const auto width = buffer->width();
        const auto height = buffer->height();
        const auto yScaledPlane = std::make_unique<uint8_t[]>(yScaledSize);
        const auto uScaledPlane = std::make_unique<uint8_t[]>(uvScaledSize);
        const auto vScaledPlane = std::make_unique<uint8_t[]>(uvScaledSize);

        I420Scale(
            buffer->DataY(), buffer->StrideY(),
            buffer->DataU(), buffer->StrideU(),
            buffer->DataV(), buffer->StrideV(),
            width, height,
            yScaledPlane.get(), desc.width,
            uScaledPlane.get(), desc.width / 2,
            vScaledPlane.get(), desc.width / 2,
            desc.width, desc.height,
            libyuv::kFilterBox
        );
        memcpy(yuv.get(), yScaledPlane.get(), yScaledSize);
        memcpy(yuv.get() + yScaledSize, uScaledPlane.get(), uvScaledSize);
        memcpy(yuv.get() + yScaledSize + uvScaledSize, vScaledPlane.get(), uvScaledSize);

        (void) dataCallback(std::move(yuv), {
            0,
            frame.rotation(),
            static_cast<uint16_t>(desc.width),
            static_cast<uint16_t>(desc.height),
        });
    }

    void JavaVideoCapturerModule::open() {
        if (running) return;
        running = true;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        const webrtc::ScopedJavaLocalRef javaModuleClass(env, env->GetObjectClass(javaModule));
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass.obj(), "open", "()V"));
    }
} // ntgcalls

#endif