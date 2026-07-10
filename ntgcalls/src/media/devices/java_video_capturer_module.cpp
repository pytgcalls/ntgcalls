//
// Created by Lauren on 19/10/24.
//

#ifdef IS_ANDROID

#include <libyuv.h>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/devices/java_video_capturer_module.hpp>
#include <sdk/android/native_api/jni/class_loader.h>
#include <sdk/android/native_api/jni/scoped_java_ref.h>
#include <wrtc/utils/java_context.hpp>

namespace ntgcalls::media::devices {
    JavaVideoCapturerModule::JavaVideoCapturerModule(const bool is_screencast, const VideoDescription& desc, BaseSink* sink): BaseIO(sink), BaseReader(sink), desc_(desc) {
        std::string device_name;
        try {
            auto source_metadata = json::parse(desc.input);
            device_name = source_metadata["id"];
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        if (device_name == "screen" && !is_screencast) {
            throw MediaDeviceError("Wrong device type");
        }
        const auto env = static_cast<JNIEnv*>(wrtc::utils::GetJNIEnv());
        const auto video_capturer_class = webrtc::GetClass(env, "io/github/pytgcalls/devices/JavaVideoCapturerModule");
        auto local_java_module = webrtc::ScopedJavaLocalRef<>::Adopt(
            env,
            env->NewObject(
                video_capturer_class.obj(),
                env->GetMethodID(video_capturer_class.obj(), "<init>", "(ZLjava/lang/String;IIIJ)V"),
                is_screencast,
                env->NewStringUTF(device_name.c_str()),
                desc.width,
                desc.height,
                desc.fps,
                reinterpret_cast<jlong>(this)
            )
        );
        java_module_ = env->NewGlobalRef(local_java_module.Release());
    }

    JavaVideoCapturerModule::~JavaVideoCapturerModule() {
        running_ = false;
        const auto env = static_cast<JNIEnv*>(wrtc::utils::GetJNIEnv());
        // ReSharper disable once CppLocalVariableMayBeConst
        const auto java_module_class = webrtc::ScopedJavaLocalRef<jclass>::Adopt(
            env,
            env->GetObjectClass(java_module_)
        );
        env->CallVoidMethod(java_module_, env->GetMethodID(java_module_class.obj(), "release", "()V"));
        env->DeleteGlobalRef(java_module_);
    }

    bool JavaVideoCapturerModule::is_supported(const bool is_screencast) {
        if (is_screencast) {
            return android_get_device_api_level() >= __ANDROID_API_L__;
        }
        return android_get_device_api_level() >= __ANDROID_API_J_MR2__;
    }

    std::vector<DeviceInfo> JavaVideoCapturerModule::get_devices() {
        const auto env = static_cast<JNIEnv*>(wrtc::utils::GetJNIEnv());
        const auto video_capturer_class = webrtc::GetClass(env, "io/github/pytgcalls/devices/JavaVideoCapturerModule");
        // ReSharper disable once CppLocalVariableMayBeConst
        jmethodID get_devices_method = env->GetStaticMethodID(video_capturer_class.obj(), "getDevices", "()Ljava/util/List;");
        const auto device_list = webrtc::ScopedJavaLocalRef<>::Adopt(
            env,
            env->CallStaticObjectMethod(video_capturer_class.obj(), get_devices_method)
        );
        const auto list_class = webrtc::GetClass(env, "java/util/List");
        // ReSharper disable once CppLocalVariableMayBeConst
        jmethodID list_size_method = env->GetMethodID(list_class.obj(), "size", "()I");
        // ReSharper disable once CppLocalVariableMayBeConst
        jmethodID list_get_method = env->GetMethodID(list_class.obj(), "get", "(I)Ljava/lang/Object;");
        const jint list_size = env->CallIntMethod(device_list.obj(), list_size_method);

        std::vector<DeviceInfo> devices;
        for (jint i = 0; i < list_size; i++) {
            auto device_info_obj = webrtc::ScopedJavaLocalRef<>::Adopt(
                env,
                env->CallObjectMethod(device_list.obj(), list_get_method, i)
            );
            auto device_info_class = webrtc::ScopedJavaLocalRef<jclass>::Adopt(
                env,
                env->GetObjectClass(device_info_obj.obj())
            );
            // ReSharper disable once CppLocalVariableMayBeConst
            jfieldID name_field_id = env->GetFieldID(device_info_class.obj(), "name", "Ljava/lang/String;");
            // ReSharper disable once CppLocalVariableMayBeConst
            jfieldID metadata_field_id = env->GetFieldID(device_info_class.obj(), "metadata", "Ljava/lang/String;");
            auto name_obj = webrtc::ScopedJavaLocalRef<jstring>::Adopt(
                env,
                reinterpret_cast<jstring>(env->GetObjectField(device_info_obj.obj(), name_field_id))
            );
            auto metadata_obj = webrtc::ScopedJavaLocalRef<jstring>::Adopt(
                env,
                reinterpret_cast<jstring>(env->GetObjectField(device_info_obj.obj(), metadata_field_id))
            );
            const auto name = env->GetStringUTFChars(name_obj.obj(), nullptr);
            const auto metadata = env->GetStringUTFChars(metadata_obj.obj(), nullptr);
            devices.emplace_back(std::string(name), std::string(metadata));
            env->ReleaseStringUTFChars(name_obj.obj(), name);
            env->ReleaseStringUTFChars(metadata_obj.obj(), metadata);
        }
        return devices;
    }

    void JavaVideoCapturerModule::on_capturer_stopped() const {
        (void) eof_callback_();
    }

    void JavaVideoCapturerModule::on_frame(const webrtc::VideoFrame& frame) const {
        const auto y_scaled_size = desc_.width * desc_.height;
        const auto uv_scaled_size = y_scaled_size / 4;
        auto yuv = bytes::make_unique_binary(y_scaled_size + uv_scaled_size * 2);
        const auto buffer = frame.video_frame_buffer()->ToI420();

        const auto width = buffer->width();
        const auto height = buffer->height();
        const auto y_scaled_plane = std::make_unique<uint8_t[]>(y_scaled_size);
        const auto u_scaled_plane = std::make_unique<uint8_t[]>(uv_scaled_size);
        const auto v_scaled_plane = std::make_unique<uint8_t[]>(uv_scaled_size);

        I420Scale(
            buffer->DataY(), buffer->StrideY(),
            buffer->DataU(), buffer->StrideU(),
            buffer->DataV(), buffer->StrideV(),
            width, height,
            y_scaled_plane.get(), desc_.width,
            u_scaled_plane.get(), desc_.width / 2,
            v_scaled_plane.get(), desc_.width / 2,
            desc_.width, desc_.height,
            libyuv::kFilterBox
        );
        std::memcpy(yuv.get(), y_scaled_plane.get(), y_scaled_size);
        std::memcpy(yuv.get() + y_scaled_size, u_scaled_plane.get(), uv_scaled_size);
        std::memcpy(yuv.get() + y_scaled_size + uv_scaled_size, v_scaled_plane.get(), uv_scaled_size);

        (void) data_callback_(std::move(yuv), {
            0,
            frame.rotation(),
            static_cast<uint16_t>(desc_.width),
            static_cast<uint16_t>(desc_.height),
        });
    }

    void JavaVideoCapturerModule::open() {
        if (running_) return;
        running_ = true;
        const auto env = static_cast<JNIEnv*>(wrtc::utils::GetJNIEnv());
        const auto java_module_class = webrtc::ScopedJavaLocalRef<jclass>::Adopt(
            env,
            env->GetObjectClass(java_module_)
        );
        env->CallVoidMethod(java_module_, env->GetMethodID(java_module_class.obj(), "open", "()V"));
    }

} // ntgcalls::media::devices

#endif