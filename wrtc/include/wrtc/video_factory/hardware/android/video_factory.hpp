//
// Created by Lauren on 15/09/24.
//

#pragma once
#ifdef IS_ANDROID
#include <sdk/android/native_api/jni/jvm.h>
#include <api/video_codecs/video_encoder_factory.h>
#include <api/video_codecs/video_decoder_factory.h>

namespace android {
    std::unique_ptr<webrtc::VideoEncoderFactory> create_video_encoder_factory(JNIEnv* env);

    std::unique_ptr<webrtc::VideoDecoderFactory> create_video_decoder_factory(JNIEnv* env);
} // android

#endif