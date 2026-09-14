//
// Created by Lauren on 14/09/26.
//

#include "utils.hpp"
#include <ntgcalls/media/devices/java_video_capturer_module.hpp>
#include <sdk/android/src/jni/video_frame.h>

extern "C" JNIEXPORT void JNICALL Java_io_github_pytgcalls_devices_JavaVideoCapturerModule_nativeCapturerStopped(JNIEnv* env, jobject thiz) {
    if (const auto ptr = getInstancePtr(env, thiz); ptr != 0) {
        reinterpret_cast<ntgcalls::media::devices::JavaVideoCapturerModule*>(ptr)->on_capturer_stopped();
    }
}

extern "C" JNIEXPORT void JNICALL Java_io_github_pytgcalls_devices_JavaVideoCapturerModule_nativeOnFrame(JNIEnv* env, jobject thiz, jobject j_frame) {
    if (const auto ptr = getInstancePtr(env, thiz); ptr != 0) {
        reinterpret_cast<ntgcalls::media::devices::JavaVideoCapturerModule*>(ptr)->on_frame(
            webrtc::jni::JavaToNativeFrame(env, webrtc::ScopedJavaLocalRef<jobject>::Adopt(env, env->NewLocalRef(j_frame)), 0)
        );
    }
}
