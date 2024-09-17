//
// Created by Laky64 on 15/09/24.
//
#undef JNIEXPORT
#define JNIEXPORT __attribute__((visibility("default")))

#include <rtc_base/logging.h>
#include <rtc_base/ssl_adapter.h>
#include <sdk/android/native_api/jni/class_loader.h>
#include <sdk/android/src/jni/jni_helpers.h>

namespace webrtc::jni {
    extern "C" jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM* jvm, void*) {
        jint ret = InitGlobalJniVariables(jvm);
        RTC_DCHECK_GE(ret, 0);
        if (ret < 0)
            return -1;
        RTC_CHECK(rtc::InitializeSSL()) << "Failed to InitializeSSL()";
        webrtc::InitClassLoader(GetEnv());
        RTC_LOG(LS_INFO) << "JNI_OnLoad ret=" << ret;
        return ret;
    }

    extern "C" void JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM*, void*) {
        RTC_CHECK(rtc::CleanupSSL()) << "Failed to CleanupSSL()";
    }
}  // namespace webrtc::jni
