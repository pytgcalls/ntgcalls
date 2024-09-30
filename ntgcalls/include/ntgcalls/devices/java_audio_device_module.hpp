//
// Created by Laky64 on 30/09/24.
//

#pragma once

#ifdef IS_ANDROID
#include <jni.h>

#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/devices/base_device_module.hpp>

namespace ntgcalls {

    class JavaAudioDeviceModule final: public BaseDeviceModule, public BaseReader {
        jobject javaModule;

    public:
        JavaAudioDeviceModule(const AudioDescription* desc, bool isCapture, BaseSink* sink);

        ~JavaAudioDeviceModule() override;

        void open() override;

        static bool isSupported();

        void onRecordedData(bytes::unique_binary data) const;
    };

} // ntgcalls

#endif