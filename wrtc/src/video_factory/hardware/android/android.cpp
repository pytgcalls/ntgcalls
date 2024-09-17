//
// Created by laky64 on 15/09/24.
//

#include <wrtc/video_factory/hardware/android/android.hpp>
#include <wrtc/video_factory/hardware/android/video_factory.hpp>

namespace android {
    void addEncoders(std::vector<wrtc::VideoEncoderConfig>& encoders, void* jniEnv) {
#ifdef IS_ANDROID
        encoders.insert(encoders.begin(), wrtc::VideoEncoderConfig(CreateVideoEncoderFactory(static_cast<JNIEnv*>(jniEnv))));
#endif
    }

    void addDecoders(std::vector<wrtc::VideoDecoderConfig>& decoders, void* jniEnv) {
#ifdef IS_ANDROID
        decoders.insert(decoders.begin(), wrtc::VideoDecoderConfig(CreateVideoDecoderFactory(static_cast<JNIEnv*>(jniEnv))));
#endif
    }
} // wrtc