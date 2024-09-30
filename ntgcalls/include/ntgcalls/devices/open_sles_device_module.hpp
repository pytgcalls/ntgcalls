//
// Created by Laky64 on 25/09/24.
//

#pragma once
#include <ntgcalls/io/base_reader.hpp>


#ifdef IS_ANDROID
#include <jni.h>
#include <SLES/OpenSLES_Android.h>
#include <api/scoped_refptr.h>
#include <sdk/android/src/jni/audio_device/opensles_common.h>
#include <ntgcalls/devices/base_device_module.hpp>

namespace ntgcalls {
    using namespace webrtc::jni;

    class OpenSLESDeviceModule final: public BaseDeviceModule, public BaseReader {
        static constexpr int kNumOfOpenSLESBuffers = 2;
        int bufferIndex = 0;
        std::unique_ptr<bytes::unique_binary[]> audioBuffers;
        std::mutex mutex;

        void initRecording();

        static void SimpleBufferQueueCallback(SLAndroidSimpleBufferQueueItf, void* context);

        [[nodiscard]] SLuint32 GetRecordState() const;

        void enqueueBuffer();

    public:
        SLEngineItf engine = nullptr;
        rtc::scoped_refptr<OpenSLEngineManager> engineManager;
        SLDataFormat_PCM pcmFormat{};
        ScopedSLObjectItf recorderObject;
        SLRecordItf recorder = nullptr;
        SLAndroidSimpleBufferQueueItf simpleBufferQueue = nullptr;

        OpenSLESDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink* sink);

        ~OpenSLESDeviceModule() override;

        static bool isSupported(JNIEnv* env, bool isCapture);

        void open() override;
    };

} // ntgcalls

#endif