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
        BaseIO(sink),
        BaseDeviceModule(desc, isCapture),
        BaseReader(sink),
        AudioMixer(sink)
    {
        engineManager = rtc::make_ref_counted<OpenSLEngineManager>();
        pcmFormat.formatType = SL_DATAFORMAT_PCM;
        pcmFormat.numChannels = desc->channelCount;
        pcmFormat.samplesPerSec = desc->sampleRate * 1000;
        pcmFormat.bitsPerSample = 16;
        pcmFormat.containerSize = 16;
        pcmFormat.channelMask = desc->channelCount == 1 ? SL_SPEAKER_FRONT_CENTER : SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        pcmFormat.endianness = SL_BYTEORDER_LITTLEENDIAN;
        const auto engineObject = engineManager->GetOpenSLEngine();
        if (engineObject == nullptr) {
            throw MediaDeviceError("Failed to create OpenSLES engine");
        }
        if ((*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engine) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES engine interface");
        }
        if (isCapture) {
            initRecording();
        } else {
            initPlayback();
        }
        registerQueueCallback();
    }

    OpenSLESDeviceModule::~OpenSLESDeviceModule() {
        std::lock_guard queueLock(queueMutex);
        running = false;
        (*simpleBufferQueue)->Clear(simpleBufferQueue);
        (*simpleBufferQueue)->RegisterCallback(simpleBufferQueue, nullptr, nullptr);
        if (isCapture) {
            (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_STOPPED);
        } else {
            (*player)->SetPlayState(player, SL_PLAYSTATE_STOPPED);
            outputMix.Reset();
        }
        deviceObject.Reset();
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
            deviceObject.Receive(),
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
        if (deviceObject->GetInterface(deviceObject.Get(), SL_IID_ANDROIDCONFIGURATION, &recorderConfig) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES audio recorder configuration interface");
        }

        constexpr SLint32 streamType = SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION;
        if ((*recorderConfig)->SetConfiguration(recorderConfig, SL_ANDROID_KEY_RECORDING_PRESET, &streamType, sizeof(SLint32)) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to set OpenSLES audio recorder configuration");
        }

        if (deviceObject->Realize(deviceObject.Get(), SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to realize OpenSLES audio recorder");
        }

        if (deviceObject->GetInterface(deviceObject.Get(), SL_IID_RECORD, &recorder) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES audio recorder interface");
        }

        if (deviceObject->GetInterface(deviceObject.Get(), SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &simpleBufferQueue) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES buffer queue interface");
        }
    }

    void OpenSLESDeviceModule::initPlayback() {
        if ((*engine)->CreateOutputMix(engine, outputMix.Receive(), 0, nullptr, nullptr) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to create OpenSLES output mix");
        }

        if (outputMix->Realize(outputMix.Get(), SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to realize OpenSLES output mix");
        }

        SLDataLocator_AndroidSimpleBufferQueue bufferQueue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            static_cast<SLuint32>(kNumOfOpenSLESBuffers)
        };
        SLDataSource audioSource = {&bufferQueue, &pcmFormat};
        SLDataLocator_OutputMix locatorOutputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMix.Get()};
        SLDataSink audioSink = {&locatorOutputMix, nullptr};
        const SLInterfaceID interfaceIds[] = {SL_IID_ANDROIDCONFIGURATION, SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
        constexpr SLboolean interfaceRequired[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

        const auto createPlayerResult = (*engine)->CreateAudioPlayer(
            engine,
            deviceObject.Receive(),
            &audioSource,
            &audioSink,
            arraysize(interfaceIds),
            interfaceIds,
            interfaceRequired
        );

        if (createPlayerResult != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to create OpenSLES audio player");
        }

        SLAndroidConfigurationItf playerConfig;
        if (deviceObject->GetInterface(deviceObject.Get(), SL_IID_ANDROIDCONFIGURATION, &playerConfig) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES audio player configuration interface");
        }

        constexpr SLint32 streamType = SL_ANDROID_STREAM_VOICE;
        if ((*playerConfig)->SetConfiguration(playerConfig, SL_ANDROID_KEY_STREAM_TYPE, &streamType, sizeof(SLint32)) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to set OpenSLES audio player configuration");
        }

        if (deviceObject->Realize(deviceObject.Get(), SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to realize OpenSLES audio player");
        }

        if (deviceObject->GetInterface(deviceObject.Get(), SL_IID_PLAY, &player) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES audio player interface");
        }

        if (deviceObject->GetInterface(deviceObject.Get(), SL_IID_BUFFERQUEUE, &simpleBufferQueue) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to get OpenSLES buffer queue interface");
        }
    }

    void OpenSLESDeviceModule::registerQueueCallback() {
        if ((*simpleBufferQueue)->RegisterCallback(simpleBufferQueue, SimpleBufferQueueCallback, this) != SL_RESULT_SUCCESS) {
            throw MediaDeviceError("Failed to register OpenSLES buffer queue callback");
        }
    }

    void OpenSLESDeviceModule::SimpleBufferQueueCallback(SLAndroidSimpleBufferQueueItf, void* context) {
        const auto thiz = static_cast<OpenSLESDeviceModule*>(context);
        auto frameSize = thiz->sink->frameSize();
        if (thiz->isCapture) {
            if (thiz->GetRecordState() != SL_RECORDSTATE_RECORDING) {
                return;
            }
            auto result = bytes::make_unique_binary(frameSize);
            memcpy(result.get(), thiz->audioBuffers[thiz->bufferIndex].get(), frameSize);
            thiz->dataCallback(std::move(result), {});
        } else {
            if (thiz->GetPlayState() != SL_PLAYSTATE_PLAYING) {
                return;
            }
            auto* audioPtr8 = reinterpret_cast<SLint8*>(thiz->audioBuffers[thiz->bufferIndex].get());
            std::lock_guard lock(thiz->queueMutex);
            if (!thiz->queue.empty()) {
                memcpy(audioPtr8, thiz->queue.front().get(), frameSize);
                thiz->queue.pop();
            }
        }
        thiz->enqueueBuffer();
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    SLuint32 OpenSLESDeviceModule::GetRecordState() const {
        SLuint32 state;
        (*recorder)->GetRecordState(recorder, &state);
        return state;
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    SLuint32 OpenSLESDeviceModule::GetPlayState() const {
        SLuint32 state;
        (*player)->GetPlayState(player, &state);
        return state;
    }

    void OpenSLESDeviceModule::enqueueBuffer() {
        (*simpleBufferQueue)->Enqueue(simpleBufferQueue, audioBuffers[bufferIndex].get(), sink->frameSize());
        bufferIndex = (bufferIndex + 1) % kNumOfOpenSLESBuffers;
    }

    void OpenSLESDeviceModule::onData(bytes::unique_binary data) {
        std::lock_guard lock(queueMutex);
        queue.push(std::move(data));
    }

    bool OpenSLESDeviceModule::isSupported(JNIEnv* env, const bool isCapture) {
        const auto jContext = webrtc::GetAppContext(env);
        if (isCapture) {
            return IsLowLatencyInputSupported(env, jContext);
        }
        return IsLowLatencyOutputSupported(env, jContext);
    }

    void OpenSLESDeviceModule::open() {
        if (running) return;
        running = true;
        audioBuffers = std::make_unique<bytes::unique_binary[]>(kNumOfOpenSLESBuffers);
        for (int i = 0; i < kNumOfOpenSLESBuffers; i++) {
            audioBuffers[i] = bytes::make_unique_binary(sink->frameSize());
            enqueueBuffer();
        }
        if (isCapture) {
            (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_RECORDING);
        } else {
            (*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING);
        }
    }
} // ntgcalls

#endif