//
// Created by Laky64 on 25/09/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/devices/open_sles_device_module.hpp>

#ifdef IS_ANDROID
#include <SLES/OpenSLES.h>
#include <api/make_ref_counted.h>
#include <sdk/android/native_api/jni/application_context_provider.h>
#include <sdk/android/src/jni/audio_device/audio_device_module.h>


namespace ntgcalls {

    OpenSLESDeviceModule::OpenSLESDeviceModule(const AudioDescription* desc, const bool isCapture, BaseSink* sink):
        BaseDeviceModule(desc, isCapture),
        BaseReader(sink)
    {
        engineManager = rtc::make_ref_counted<OpenSLEngineManager>();
        pcmFormat.formatType = SL_DATAFORMAT_PCM;
        pcmFormat.numChannels = desc->channelCount;
        pcmFormat.samplesPerSec = desc->sampleRate * 1000;
        pcmFormat.bitsPerSample = desc->bitsPerSample;
        pcmFormat.containerSize = desc->bitsPerSample;
        pcmFormat.channelMask = desc->channelCount == 1 ? SL_SPEAKER_FRONT_CENTER : SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        pcmFormat.endianness = SL_BYTEORDER_LITTLEENDIAN;
        const auto engineObject = engineManager->GetOpenSLEngine();
        if (engineObject == nullptr) {
            throw MediaDeviceError("Failed to create OpenSLES engine");
        }
        RTC_LOG(LS_ERROR) << desc->channelCount << " " << desc->sampleRate << " " << desc->bitsPerSample;
        if ((*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engine) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES engine interface");
        }
        if (isCapture) {
            initRecording();
        } else {
            //initPlayback();
        }
    }

    OpenSLESDeviceModule::~OpenSLESDeviceModule() {
        (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_STOPPED);
        (*simpleBufferQueue)->Clear(simpleBufferQueue);
        (*simpleBufferQueue)->RegisterCallback(simpleBufferQueue, nullptr, nullptr);
        recorderObject.Reset();
        engine = nullptr;
    }

    void OpenSLESDeviceModule::initRecording() {
        SLDataLocator_IODevice micLocator = {
            SL_DATALOCATOR_IODEVICE,
            SL_IODEVICE_AUDIOINPUT,
            SL_DEFAULTDEVICEID_AUDIOINPUT,
            nullptr
        };

        SLDataSource audioSource = {&micLocator, nullptr};
        SLDataLocator_AndroidSimpleBufferQueue bufferQueue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            static_cast<SLuint32>(kNumOfOpenSLESBuffers)
        };
        SLDataSink audioSink = {&bufferQueue, &pcmFormat};
        SLInterfaceID interfaceId[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION};
        constexpr SLboolean interfaceRequired[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

        const auto createRecorderResult = (*engine)->CreateAudioRecorder(
            engine,
            recorderObject.Receive(),
            &audioSource,
            &audioSink,
            arraysize(interfaceId),
            interfaceId,
            interfaceRequired
        );
        if (createRecorderResult != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to create OpenSLES audio recorder");
        }

        SLAndroidConfigurationItf recorderConfig;
        if (recorderObject->GetInterface(recorderObject.Get(), SL_IID_ANDROIDCONFIGURATION, &recorderConfig) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES audio recorder configuration interface");
        }

        constexpr SLint32 streamType = SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION;
        if ((*recorderConfig)->SetConfiguration(recorderConfig, SL_ANDROID_KEY_RECORDING_PRESET, &streamType, sizeof(SLint32)) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to set OpenSLES audio recorder configuration");
        }

        if (recorderObject->Realize(recorderObject.Get(), SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to realize OpenSLES audio recorder");
        }

        if (recorderObject->GetInterface(recorderObject.Get(), SL_IID_RECORD, &recorder) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES audio recorder interface");
        }

        if (recorderObject->GetInterface(recorderObject.Get(), SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &simpleBufferQueue) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES audio recorder interface");
        }

        if ((*simpleBufferQueue)->RegisterCallback(simpleBufferQueue, SimpleBufferQueueCallback, this) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to register OpenSLES audio recorder callback");
        }
    }

    void OpenSLESDeviceModule::SimpleBufferQueueCallback(SLAndroidSimpleBufferQueueItf, void* context) {
        const auto thiz = static_cast<OpenSLESDeviceModule*>(context);
        if (thiz->GetRecordState() != SL_RECORDSTATE_RECORDING) {
            return;
        }
        auto frameSize = thiz->sink->frameSize();
        auto result = bytes::make_unique_binary(frameSize);
        memcpy(result.get(), thiz->audioBuffers[thiz->bufferIndex].get(), frameSize);
        thiz->dataCallback(std::move(result));
        thiz->enqueueBuffer();
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    SLuint32 OpenSLESDeviceModule::GetRecordState() const {
        SLuint32 state;
        (*recorder)->GetRecordState(recorder, &state);
        return state;
    }

    void OpenSLESDeviceModule::enqueueBuffer() {
        (*simpleBufferQueue)->Enqueue(simpleBufferQueue, audioBuffers[bufferIndex].get(), sink->frameSize());
        bufferIndex = (bufferIndex + 1) % kNumOfOpenSLESBuffers;
    }

    bool OpenSLESDeviceModule::isSupported(JNIEnv* env, const bool isCapture) {
        const auto jContext = webrtc::GetAppContext(env);
        if (isCapture) {
            return IsLowLatencyInputSupported(env, jContext);
        }
        return IsLowLatencyOutputSupported(env, jContext);
    }

    void OpenSLESDeviceModule::open() {
        audioBuffers = std::make_unique<bytes::unique_binary[]>(kNumOfOpenSLESBuffers);
        for (int i = 0; i < kNumOfOpenSLESBuffers; i++) {
            audioBuffers[i] = bytes::make_unique_binary(sink->frameSize());
            enqueueBuffer();
        }
        (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_RECORDING);
    }
} // ntgcalls

#endif