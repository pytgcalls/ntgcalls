//
// Created by Laky64 on 15/09/24.
//

#include <wrtc/utils/java_context.hpp>

#ifdef IS_ANDROID
#include <modules/utility/include/jvm_android.h>
#include <sdk/android/native_api/jni/jvm.h>
#include <sdk/android/src/jni/jvm.h>
#endif

namespace wrtc {
    void* GetJNIEnv() {
#ifdef IS_ANDROID
        void* env = webrtc::AttachCurrentThreadIfNeeded();
        static bool jvm_initialized = false;
        if (!jvm_initialized) {
            webrtc::JVM::Initialize(webrtc::jni::GetJVM());
            jvm_initialized = true;
        }
        return env;
#else
        return nullptr;
#endif
    }
} // wrtc
