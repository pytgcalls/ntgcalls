//
// Created by Laky64 on 15/09/24.
//

#pragma once
#ifdef IS_ANDROID
#include <sdk/android/native_api/jni/jvm.h>
#include <api/video_codecs/video_encoder_factory.h>
#include <api/video_codecs/video_decoder_factory.h>

namespace android {
    std::unique_ptr<webrtc::VideoEncoderFactory> CreateVideoEncoderFactory(JNIEnv* env);

    std::unique_ptr<webrtc::VideoDecoderFactory> CreateVideoDecoderFactory(JNIEnv* env);
} // android

#endif