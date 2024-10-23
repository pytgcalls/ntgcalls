//
// Created by Laky64 on 19/10/24.
//

#ifdef IS_ANDROID

#include <libyuv.h>
#include <ntgcalls/exceptions.hpp>
#include <wrtc/utils/java_context.hpp>
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
        const auto videoCapturerClass = env->FindClass("org/pytgcalls/ntgcalls/devices/JavaVideoCapturerModule");
        // ReSharper disable once CppLocalVariableMayBeConst
        auto localJavaModule = env->NewObject(
            videoCapturerClass,
            env->GetMethodID(videoCapturerClass, "<init>", "(ZLjava/lang/String;IIIJ)V"),
            isScreencast,
            env->NewStringUTF(deviceName.c_str()),
            desc.width,
            desc.height,
            desc.fps,
            reinterpret_cast<jlong>(this)
        );
        javaModule = env->NewGlobalRef(localJavaModule);
        env->DeleteLocalRef(videoCapturerClass);
        env->DeleteLocalRef(localJavaModule);
    }

    JavaVideoCapturerModule::~JavaVideoCapturerModule() {
        running = false;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        jclass javaModuleClass = env->GetObjectClass(javaModule);
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass, "release", "()V"));
        env->DeleteGlobalRef(javaModule);
        env->DeleteLocalRef(javaModuleClass);
    }

    bool JavaVideoCapturerModule::IsSupported(const bool isScreencast) {
        if (isScreencast) {
            return android_get_device_api_level() >= __ANDROID_API_L__;
        }
        return android_get_device_api_level() >= __ANDROID_API_J_MR2__;
    }

    std::vector<DeviceInfo> JavaVideoCapturerModule::getDevices() {
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        const auto videoCapturerClass = env->FindClass("org/pytgcalls/ntgcalls/devices/JavaVideoCapturerModule");
        // ReSharper disable once CppLocalVariableMayBeConst
        jmethodID getDevicesMethod = env->GetStaticMethodID(videoCapturerClass, "getDevices", "()Ljava/util/List;");
        const auto deviceList = env->CallStaticObjectMethod(videoCapturerClass, getDevicesMethod);
        // ReSharper disable once CppLocalVariableMayBeConst
        jclass listClass = env->FindClass("java/util/List");
        // ReSharper disable once CppLocalVariableMayBeConst
        jmethodID listSizeMethod = env->GetMethodID(listClass, "size", "()I");
        // ReSharper disable once CppLocalVariableMayBeConst
        jmethodID listGetMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
        const jint listSize = env->CallIntMethod(deviceList, listSizeMethod);

        std::vector<DeviceInfo> devices;
        for (jint i = 0; i < listSize; i++) {
            const auto deviceInfoObj = env->CallObjectMethod(deviceList, listGetMethod, i);
            const auto deviceInfoClass = env->GetObjectClass(deviceInfoObj);
            // ReSharper disable once CppLocalVariableMayBeConst
            jfieldID nameFieldID = env->GetFieldID(deviceInfoClass, "name", "Ljava/lang/String;");
            // ReSharper disable once CppLocalVariableMayBeConst
            jfieldID metadataFieldID = env->GetFieldID(deviceInfoClass, "metadata", "Ljava/lang/String;");
            const auto nameObj = reinterpret_cast<jstring>(env->GetObjectField(deviceInfoObj, nameFieldID));
            const auto metadataObj = reinterpret_cast<jstring>(env->GetObjectField(deviceInfoObj, metadataFieldID));
            const auto name = env->GetStringUTFChars(nameObj, nullptr);
            const auto metadata = env->GetStringUTFChars(metadataObj, nullptr);
            devices.emplace_back(std::string(name), std::string(metadata));
            env->ReleaseStringUTFChars(nameObj, name);
            env->ReleaseStringUTFChars(metadataObj, metadata);
            env->DeleteLocalRef(deviceInfoClass);
        }
        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(videoCapturerClass);
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
            .rotation = frame.rotation()
        });
    }

    void JavaVideoCapturerModule::open() {
        if (running) return;
        running = true;
        const auto env = static_cast<JNIEnv*>(wrtc::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        jclass javaModuleClass = env->GetObjectClass(javaModule);
        env->CallVoidMethod(javaModule, env->GetMethodID(javaModuleClass, "open", "()V"));
        env->DeleteLocalRef(javaModuleClass);
    }
} // ntgcalls

#endif