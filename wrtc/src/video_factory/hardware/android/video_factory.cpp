//
// Created by Laky64 on 15/09/24.
//

#ifdef IS_ANDROID
#include <wrtc/video_factory/hardware/android/video_factory.hpp>
#include <sdk/android/native_api/codecs/wrapper.h>
#include <sdk/android/native_api/jni/class_loader.h>
#include <sdk/android/native_api/jni/scoped_java_ref.h>

namespace android {
    std::unique_ptr<webrtc::VideoEncoderFactory> CreateVideoEncoderFactory(JNIEnv* env) {
        const webrtc::ScopedJavaLocalRef<jclass> videoEncoderFactory = webrtc::GetClass(env, "org/webrtc/DefaultVideoEncoderFactory");
        return webrtc::JavaToNativeVideoEncoderFactory(
            env,
            env->NewObject(
                videoEncoderFactory.obj(),
                env->GetMethodID(
                    videoEncoderFactory.obj(),
                    "<init>",
                    "(Lorg/webrtc/EglBase$Context;ZZ)V"
                ),
                nullptr,
                true,
                false
            )
        );
    }

    std::unique_ptr<webrtc::VideoDecoderFactory> CreateVideoDecoderFactory(JNIEnv* env) {
        const webrtc::ScopedJavaLocalRef<jclass> videoDecoderFactory = webrtc::GetClass(env, "org/webrtc/DefaultVideoDecoderFactory");
        return webrtc::JavaToNativeVideoDecoderFactory(
            env,
            env->NewObject(
                videoDecoderFactory.obj(),
                env->GetMethodID(
                    videoDecoderFactory.obj(),
                    "<init>",
                    "(Lorg/webrtc/EglBase$Context;)V"
                ),
                nullptr
            )
        );
    }
} // android
#endif