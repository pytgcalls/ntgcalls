//
// Created by Laky64 on 25/09/24.
//

#pragma once

#ifdef IS_ANDROID
#include <jni.h>
#include <SLES/OpenSLES_Android.h>
#include <api/scoped_refptr.h>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/base_device_module.hpp>
#include <sdk/android/src/jni/audio_device/opensles_common.h>
#include <ntgcalls/io/audio_mixer.hpp>

namespace ntgcalls {
    using namespace webrtc::jni;

    class OpenSLESDeviceModule final: public BaseDeviceModule, public BaseReader, public AudioMixer {
        static constexpr int kNumOfOpenSLESBuffers = 2;
        int bufferIndex = 0;
        std::unique_ptr<bytes::unique_binary[]> audioBuffers;
        std::mutex mutex;
        SLEngineItf engine = nullptr;
        rtc::scoped_refptr<OpenSLEngineManager> engineManager;
        SLDataFormat_PCM pcmFormat{};
        ScopedSLObjectItf deviceObject;
        std::mutex queueMutex;
        std::queue<bytes::unique_binary> queue;

        SLRecordItf recorder = nullptr;
        SLPlayItf player = nullptr;
        SLAndroidSimpleBufferQueueItf simpleBufferQueue = nullptr;
        ScopedSLObjectItf outputMix;

        void initRecording();

        void initPlayback();

        static void SimpleBufferQueueCallback(SLAndroidSimpleBufferQueueItf, void* context);

        void registerQueueCallback();

        [[nodiscard]] SLuint32 GetRecordState() const;

        [[nodiscard]] SLuint32 GetPlayState() const;

        void enqueueBuffer();

        void onData(bytes::unique_binary data) override;

    public:
        OpenSLESDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink* sink);

        ~OpenSLESDeviceModule() override;

        static bool isSupported(JNIEnv* env, bool isCapture);

        void open() override;
    };

} // ntgcalls

#endif