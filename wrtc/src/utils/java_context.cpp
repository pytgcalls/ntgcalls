//
// Created by Laky64 on 15/09/24.
//

#include "wrtc/utils/java_context.hpp"

#include <rtc_base/logging.h>

#ifdef IS_ANDROID
#include <modules/utility/include/jvm_android.h>
#include <sdk/android/native_api/jni/jvm.h>
#include <sdk/android/src/jni/jvm.h>
#endif

namespace wrtc {
    void* GetJNIEnv() {
#ifdef IS_ANDROID
        RTC_LOG(LS_ERROR) << "GetJNIEnv::1";
        void* env = webrtc::AttachCurrentThreadIfNeeded();
        RTC_LOG(LS_ERROR) << "GetJNIEnv::2" << env;
        static bool jvm_initialized = false;
        if (!jvm_initialized) {
            RTC_LOG(LS_ERROR) << "GetJNIEnv::3" << env;
            webrtc::JVM::Initialize(webrtc::jni::GetJVM());
            RTC_LOG(LS_ERROR) << "GetJNIEnv::4" << env;
            jvm_initialized = true;
        }
        RTC_LOG(LS_ERROR) << "GetJNIEnv::5" << env;
        return env;
#else
        return nullptr;
#endif
    }
} // wrtc
