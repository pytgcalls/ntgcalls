//
// Created by Laky64 on 15/09/24.
//

#include <wrtc/utils/java_context.hpp>

#ifdef IS_ANDROID
#include <sdk/android/native_api/jni/jvm.h>
#include <sdk/android/src/jni/jvm.h>
#endif

namespace wrtc {
    void* GetJNIEnv() {
#ifdef IS_ANDROID
        return webrtc::AttachCurrentThreadIfNeeded();
#else
        return nullptr;
#endif
    }
} // wrtc
