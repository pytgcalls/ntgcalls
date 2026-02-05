//
// Created by Laky64 on 15/09/24.
//
#undef JNIEXPORT
#include <rtc_base/logging.h>
#include <rtc_base/ssl_adapter.h>
#include <sdk/android/native_api/base/init.h>
#include <sdk/android/native_api/jni/class_loader.h>
#include <modules/utility/include/jvm_android.h>
#include <ntgcalls/utils/log_sink_impl.hpp>
#include <sdk/android/src/jni/jni_helpers.h>

namespace webrtc::jni {
    extern "C" jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM* jvm, void*) {
        webrtc::InitAndroid(jvm);
        webrtc::JVM::Initialize(jvm);
        RTC_CHECK(webrtc::InitializeSSL()) << "Failed to InitializeSSL()";
        return JNI_VERSION_1_6;
    }

    extern "C" void JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM*, void*) {
        RTC_CHECK(webrtc::CleanupSSL()) << "Failed to CleanupSSL()";
    }
}  // namespace webrtc::jni
