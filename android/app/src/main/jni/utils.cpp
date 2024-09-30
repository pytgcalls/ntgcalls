#include "utils.hpp"

ntgcalls::NTgCalls* getInstance(JNIEnv *env, jobject obj) {
    auto ptr = getInstancePtr(env, obj);
    if (ptr != 0) {
        return reinterpret_cast<ntgcalls::NTgCalls*>(ptr);
    }
    return nullptr;
}

ntgcalls::JavaAudioDeviceModule* getInstanceAudioCapture(JNIEnv *env, jobject obj) {
    auto ptr = getInstancePtr(env, obj);
    if (ptr != 0) {
        return reinterpret_cast<ntgcalls::JavaAudioDeviceModule*>(ptr);
    }
    return nullptr;
}

jlong getInstancePtr(JNIEnv *env, jobject obj) {
    jclass clazz = env->GetObjectClass(obj);
    jlong ptr = env->GetLongField(obj,  env->GetFieldID(clazz, "nativePointer", "J"));
    env->DeleteLocalRef(clazz);
    return ptr;
}

ntgcalls::AudioDescription parseAudioDescription(JNIEnv *env, jobject audioDescription) {
    jclass audioDescriptionClass = env->GetObjectClass(audioDescription);
    jfieldID inputField = env->GetFieldID(audioDescriptionClass, "input", "Ljava/lang/String;");
    jfieldID inputModeField = env->GetFieldID(audioDescriptionClass, "inputMode", "I");
    jfieldID sampleRateField = env->GetFieldID(audioDescriptionClass, "sampleRate", "I");
    jfieldID bitsPerSampleField = env->GetFieldID(audioDescriptionClass, "bitsPerSample", "I");
    jfieldID channelCountField = env->GetFieldID(audioDescriptionClass, "channelCount", "I");

    auto input = (jstring) env->GetObjectField(audioDescription, inputField);
    auto inputMode = env->GetIntField(audioDescription, inputModeField);
    auto sampleRate = static_cast<uint32_t>(env->GetIntField(audioDescription, sampleRateField));
    auto bitsPerSample = static_cast<uint8_t>(env->GetIntField(audioDescription, bitsPerSampleField));
    auto channelCount = static_cast<uint8_t>(env->GetIntField(audioDescription, channelCountField));

    ntgcalls::AudioDescription result = {
        parseInputMode(inputMode),
        sampleRate,
        bitsPerSample,
        channelCount,
        parseString(env, input)
    };
    env->DeleteLocalRef(audioDescriptionClass);
    env->DeleteLocalRef(input);
    return result;
}

ntgcalls::VideoDescription parseVideoDescription(JNIEnv *env, jobject videoDescription) {
    jclass videoDescriptionClass = env->GetObjectClass(videoDescription);
    jfieldID inputField = env->GetFieldID(videoDescriptionClass, "input", "Ljava/lang/String;");
    jfieldID inputModeField = env->GetFieldID(videoDescriptionClass, "inputMode", "I");
    jfieldID widthField = env->GetFieldID(videoDescriptionClass, "width", "I");
    jfieldID heightField = env->GetFieldID(videoDescriptionClass, "height", "I");
    jfieldID fpsField = env->GetFieldID(videoDescriptionClass, "fps", "I");

    auto input = (jstring) env->GetObjectField(videoDescription, inputField);
    auto inputMode = env->GetIntField(videoDescription, inputModeField);
    auto width = static_cast<uint16_t>(env->GetIntField(videoDescription, widthField));
    auto height = static_cast<uint16_t>(env->GetIntField(videoDescription, heightField));
    auto fps = static_cast<uint8_t>(env->GetIntField(videoDescription, fpsField));

    ntgcalls::VideoDescription result = {
        parseInputMode(inputMode),
        width,
        height,
        fps,
        parseString(env, input)
    };
    env->DeleteLocalRef(videoDescriptionClass);
    env->DeleteLocalRef(input);
    return result;
}

ntgcalls::MediaDescription parseMediaDescription(JNIEnv *env, jobject mediaDescription) {
    if (mediaDescription == nullptr) {
        return {
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt
        };
    }
    jclass mediaDescriptionClass = env->GetObjectClass(mediaDescription);
    jfieldID micField = env->GetFieldID(mediaDescriptionClass, "microphone", "Lorg/pytgcalls/ntgcalls/media/AudioDescription;");
    jfieldID speakerField = env->GetFieldID(mediaDescriptionClass, "speaker", "Lorg/pytgcalls/ntgcalls/media/AudioDescription;");
    jfieldID cameraField = env->GetFieldID(mediaDescriptionClass, "camera", "Lorg/pytgcalls/ntgcalls/media/VideoDescription;");
    jfieldID screenField = env->GetFieldID(mediaDescriptionClass, "screen", "Lorg/pytgcalls/ntgcalls/media/VideoDescription;");

    auto microphone = env->GetObjectField(mediaDescription, micField);
    auto speaker = env->GetObjectField(mediaDescription, speakerField);
    auto camera = env->GetObjectField(mediaDescription, cameraField);
    auto screen = env->GetObjectField(mediaDescription, screenField);

    ntgcalls::MediaDescription result = {
        microphone != nullptr ? std::optional(parseAudioDescription(env, microphone)) : std::nullopt,
        speaker != nullptr ? std::optional(parseAudioDescription(env, speaker)) : std::nullopt,
        camera != nullptr ? std::optional(parseVideoDescription(env, camera)) : std::nullopt,
        screen != nullptr ? std::optional(parseVideoDescription(env, screen)) : std::nullopt
    };
    env->DeleteLocalRef(mediaDescriptionClass);
    env->DeleteLocalRef(microphone);
    env->DeleteLocalRef(speaker);
    env->DeleteLocalRef(camera);
    env->DeleteLocalRef(screen);
    return result;
}

ntgcalls::BaseMediaDescription::InputMode parseInputMode(jint inputMode) {
    ntgcalls::BaseMediaDescription::InputMode res = ntgcalls::BaseMediaDescription::InputMode::Unknown;
    if (auto check = ntgcalls::BaseMediaDescription::InputMode::File;inputMode == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::InputMode::Shell;inputMode == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::InputMode::FFmpeg;inputMode == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::InputMode::Device;inputMode == check) {
        res |= check;
    }
    return res;
}

ntgcalls::DhConfig parseDhConfig(JNIEnv *env, jobject dhConfig) {
    if (dhConfig == nullptr) {
        throw ntgcalls::InvalidParams("DHConfig is required");
    }
    jclass dhConfigClass = env->GetObjectClass(dhConfig);
    jfieldID gFieldID = env->GetFieldID(dhConfigClass, "g", "I");
    jfieldID pFieldID = env->GetFieldID(dhConfigClass, "p", "[B");
    jfieldID randomFieldID = env->GetFieldID(dhConfigClass, "random", "[B");

    auto g = static_cast<int32_t>(env->GetIntField(dhConfig, gFieldID));
    auto pArray = reinterpret_cast<jbyteArray>(env->GetObjectField(dhConfig, pFieldID));
    auto randomArray = reinterpret_cast<jbyteArray>(env->GetObjectField(dhConfig, randomFieldID));

    ntgcalls::DhConfig result = {
        g,
        parseByteArray(env, pArray),
        parseByteArray(env, randomArray)
    };
    env->DeleteLocalRef(dhConfigClass);
    env->DeleteLocalRef(pArray);
    env->DeleteLocalRef(randomArray);
    return result;
}

std::string parseString(JNIEnv *env, jstring string) {
    if (string == nullptr) {
        return {};
    }
    return {env->GetStringUTFChars(string, nullptr)};
}

jstring parseJString(JNIEnv *env, const std::string &string) {
    return env->NewStringUTF(string.c_str());
}

bytes::vector parseByteArray(JNIEnv *env, jbyteArray byteArray) {
    if (byteArray == nullptr) {
        return {};
    }
    jsize length = env->GetArrayLength(byteArray);
    auto byteBuffer =  (uint8_t *) env->GetByteArrayElements(byteArray, nullptr);
    bytes::vector result(length);
    memcpy(&result[0], byteBuffer, length);
    env->ReleaseByteArrayElements(byteArray, (jbyte *) byteBuffer, JNI_ABORT);
    return result;
}

jbyteArray parseJByteArray(JNIEnv *env, const bytes::vector& byteArray) {
    jbyteArray result = env->NewByteArray( static_cast<jsize>(byteArray.size()));
    env->SetByteArrayRegion(result, 0,  static_cast<jsize>(byteArray.size()), reinterpret_cast<const jbyte*>(byteArray.data()));
    return result;
}

bytes::binary parseBinary(JNIEnv *env, jbyteArray byteArray) {
    if (byteArray == nullptr) {
        return {};
    }
    jsize length = env->GetArrayLength(byteArray);
    auto byteBuffer =  (uint8_t *) env->GetByteArrayElements(byteArray, nullptr);
    bytes::binary result(length);
    memcpy(&result[0], byteBuffer, length);
    env->ReleaseByteArrayElements(byteArray, (jbyte *) byteBuffer, JNI_ABORT);
    return result;
}

jbyteArray parseJBinary(JNIEnv *env, const bytes::binary& binary) {
    jbyteArray result = env->NewByteArray( static_cast<jsize>(binary.size()));
    env->SetByteArrayRegion(result, 0,  static_cast<jsize>(binary.size()), reinterpret_cast<const jbyte*>(binary.data()));
    return result;
}

bytes::unique_binary parseUniqueBinary(JNIEnv *env, jbyteArray byteArray) {
    if (byteArray == nullptr) {
        return {};
    }
    jsize length = env->GetArrayLength(byteArray);
    auto byteBuffer =  (uint8_t *) env->GetByteArrayElements(byteArray, nullptr);
    auto result = bytes::make_unique_binary(length);
    memcpy(result.get(), byteBuffer, length);
    env->ReleaseByteArrayElements(byteArray, (jbyte *) byteBuffer, JNI_ABORT);
    return result;
}

jobject parseAuthParams(JNIEnv *env, const ntgcalls::AuthParams& authParams) {
    jclass authParamsClass = env->FindClass("org/pytgcalls/ntgcalls/p2p/AuthParams");
    jmethodID constructor = env->GetMethodID(authParamsClass, "<init>", "([BJ)V");
    jbyteArray g_a_or_b = parseJByteArray(env, authParams.g_a_or_b);
    jobject authParamsObject = env->NewObject(authParamsClass, constructor, g_a_or_b, authParams.key_fingerprint);
    env->DeleteLocalRef(authParamsClass);
    env->DeleteLocalRef(g_a_or_b);
    return authParamsObject;
}

std::vector<std::string> parseStringList(JNIEnv *env, jobject list) {
    if (list == nullptr) {
        return {};
    }
    jclass listClass = env->GetObjectClass(list);
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    std::vector<std::string> result;
    jint size = env->CallIntMethod(list, sizeMethod);
    for (int i = 0; i < size; i++) {
        auto element = reinterpret_cast<jstring>(env->CallObjectMethod(list, getMethod, i));
        result.push_back(parseString(env, element));
        env->DeleteLocalRef(element);
    }
    env->DeleteLocalRef(listClass);
    return result;
}

jobject parseJStringList(JNIEnv *env, const std::vector<std::string> &list) {
    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID constructor = env->GetMethodID(arrayListClass, "<init>", "(I)V");
    jobject result = env->NewObject(arrayListClass, constructor, static_cast<jint>(list.size()));
    jmethodID addMethod = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
    for (const auto &element : list) {
        auto string = parseJString(env, element);
        env->CallBooleanMethod(result, addMethod, string);
        env->DeleteLocalRef(string);
    }
    env->DeleteLocalRef(arrayListClass);
    return result;
}

ntgcalls::RTCServer parseRTCServer(JNIEnv *env, jobject rtcServer) {
    jclass dhRTCServerClass = env->GetObjectClass(rtcServer);
    jfieldID idFieldID = env->GetFieldID(dhRTCServerClass, "id", "J");
    jfieldID ipv4FieldID = env->GetFieldID(dhRTCServerClass, "ipv4", "Ljava/lang/String;");
    jfieldID ipv6FieldID = env->GetFieldID(dhRTCServerClass, "ipv6", "Ljava/lang/String;");
    jfieldID portFieldID = env->GetFieldID(dhRTCServerClass, "port", "I");
    jfieldID usernameFieldID = env->GetFieldID(dhRTCServerClass, "username", "Ljava/lang/String;");
    jfieldID passwordFieldID = env->GetFieldID(dhRTCServerClass, "password", "Ljava/lang/String;");
    jfieldID turnFieldID = env->GetFieldID(dhRTCServerClass, "turn", "Z");
    jfieldID stunFieldID = env->GetFieldID(dhRTCServerClass, "stun", "Z");
    jfieldID tcpFieldID = env->GetFieldID(dhRTCServerClass, "tcp", "Z");
    jfieldID peerTagFieldID = env->GetFieldID(dhRTCServerClass, "peerTag", "[B");

    auto id =  static_cast<uint64_t>(env->GetLongField(rtcServer, idFieldID));
    auto ipv4 = reinterpret_cast<jstring>(env->GetObjectField(rtcServer, ipv4FieldID));
    auto ipv6 =reinterpret_cast<jstring>(env->GetObjectField(rtcServer, ipv6FieldID));
    auto port = static_cast<uint16_t>(env->GetIntField(rtcServer, portFieldID));
    auto username = reinterpret_cast<jstring>(env->GetObjectField(rtcServer, usernameFieldID));
    auto password = reinterpret_cast<jstring>(env->GetObjectField(rtcServer, passwordFieldID));
    auto turn = static_cast<bool>(env->GetBooleanField(rtcServer, turnFieldID));
    auto stun = static_cast<bool>(env->GetBooleanField(rtcServer, stunFieldID));
    auto tcp = static_cast<bool>(env->GetBooleanField(rtcServer, tcpFieldID));
    auto peerTag = reinterpret_cast<jbyteArray>(env->GetObjectField(rtcServer, peerTagFieldID));

    ntgcalls::RTCServer result = {
        id,
        parseString(env, ipv4),
        parseString(env, ipv6),
        port,
        username != nullptr ? std::optional(parseString(env, username)) : std::nullopt,
        password != nullptr ? std::optional(parseString(env, password)) : std::nullopt,
        turn,
        stun,
        tcp,
        peerTag != nullptr ? std::optional(parseBinary(env, peerTag)) : std::nullopt
    };
    env->DeleteLocalRef(dhRTCServerClass);
    env->DeleteLocalRef(ipv4);
    env->DeleteLocalRef(ipv6);
    if (username != nullptr) {
        env->DeleteLocalRef(username);
    }
    if (password != nullptr) {
        env->DeleteLocalRef(password);
    }
    if (peerTag != nullptr) {
        env->DeleteLocalRef(peerTag);
    }
    return result;
}

std::vector<ntgcalls::RTCServer> parseRTCServerList(JNIEnv *env, jobject list) {
    if (list == nullptr) {
        return {};
    }
    jclass listClass = env->GetObjectClass(list);
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    std::vector<ntgcalls::RTCServer> result;
    for (int i = 0; i < env->CallIntMethod(list, sizeMethod); i++) {
        auto element = reinterpret_cast<jobject>(env->CallObjectMethod(list, getMethod, i));
        result.push_back(parseRTCServer(env, element));
        env->DeleteLocalRef(element);
    }
    env->DeleteLocalRef(listClass);
    return result;
}

jobject parseMediaState(JNIEnv *env, ntgcalls::MediaState mediaState) {
    jclass mediaStateClass = env->FindClass("org/pytgcalls/ntgcalls/media/MediaState");
    jmethodID constructor = env->GetMethodID(mediaStateClass, "<init>", "(ZZZ)V");
    jobject result = env->NewObject(mediaStateClass, constructor, mediaState.muted, mediaState.videoPaused, mediaState.videoStopped);
    env->DeleteLocalRef(mediaStateClass);
    return result;
}

jobject parseProtocol(JNIEnv *env, const ntgcalls::Protocol &protocol) {
    jclass protocolClass = env->FindClass("org/pytgcalls/ntgcalls/p2p/Protocol");
    jmethodID constructor = env->GetMethodID(protocolClass, "<init>", "(IIZZLjava/util/List;)V");
    jobject libraryVersions = parseJStringList(env, protocol.library_versions);
    jobject result = env->NewObject(protocolClass, constructor, protocol.min_layer, protocol.max_layer, protocol.udp_p2p, protocol.udp_reflector, libraryVersions);
    env->DeleteLocalRef(protocolClass);
    return result;
}

jobject parseStreamType(JNIEnv *env, ntgcalls::StreamManager::Type type) {
    jclass streamTypeClass = env->FindClass("org/pytgcalls/ntgcalls/media/StreamType");
    jfieldID audioField = env->GetStaticFieldID(streamTypeClass, "AUDIO", "Lorg/pytgcalls/ntgcalls/media/StreamType;");
    jfieldID videoField = env->GetStaticFieldID(streamTypeClass, "VIDEO", "Lorg/pytgcalls/ntgcalls/media/StreamType;");

    jobject result;
    switch (type) {
        case ntgcalls::StreamManager::Type::Audio:
            result = env->GetStaticObjectField(streamTypeClass, audioField);
            break;
        case ntgcalls::StreamManager::Type::Video:
            result = env->GetStaticObjectField(streamTypeClass, videoField);
            break;
    }
    env->DeleteLocalRef(streamTypeClass);
    return result;
}

jobject parseConnectionState(JNIEnv *env, ntgcalls::CallInterface::ConnectionState state) {
    jclass connectionStateClass = env->FindClass("org/pytgcalls/ntgcalls/ConnectionState");
    jfieldID connectingField = env->GetStaticFieldID(connectionStateClass, "CONNECTING", "Lorg/pytgcalls/ntgcalls/ConnectionState;");
    jfieldID connectedField = env->GetStaticFieldID(connectionStateClass, "CONNECTED", "Lorg/pytgcalls/ntgcalls/ConnectionState;");
    jfieldID failedField = env->GetStaticFieldID(connectionStateClass, "FAILED", "Lorg/pytgcalls/ntgcalls/ConnectionState;");
    jfieldID timeoutField = env->GetStaticFieldID(connectionStateClass, "TIMEOUT", "Lorg/pytgcalls/ntgcalls/ConnectionState;");
    jfieldID closedField = env->GetStaticFieldID(connectionStateClass, "CLOSED", "Lorg/pytgcalls/ntgcalls/ConnectionState;");

    jobject result;
    switch (state) {
        case ntgcalls::CallInterface::ConnectionState::Connecting:
            result = env->GetStaticObjectField(connectionStateClass, connectingField);
            break;
        case ntgcalls::CallInterface::ConnectionState::Connected:
            result = env->GetStaticObjectField(connectionStateClass, connectedField);
            break;
        case ntgcalls::CallInterface::ConnectionState::Failed:
            result = env->GetStaticObjectField(connectionStateClass, failedField);
            break;
        case ntgcalls::CallInterface::ConnectionState::Timeout:
            result = env->GetStaticObjectField(connectionStateClass, timeoutField);
            break;
        case ntgcalls::CallInterface::ConnectionState::Closed:
            result = env->GetStaticObjectField(connectionStateClass, closedField);
            break;
    }
    env->DeleteLocalRef(connectionStateClass);
    return result;
}

jobject parseStreamStatus(JNIEnv *env, ntgcalls::StreamManager::Status status) {
    jclass streamStatusClass = env->FindClass("org/pytgcalls/ntgcalls/media/StreamStatus");
    jfieldID playingField = env->GetStaticFieldID(streamStatusClass, "ACTIVE", "Lorg/pytgcalls/ntgcalls/media/StreamStatus;");
    jfieldID pausedField = env->GetStaticFieldID(streamStatusClass, "PAUSED", "Lorg/pytgcalls/ntgcalls/media/StreamStatus;");
    jfieldID idlingField = env->GetStaticFieldID(streamStatusClass, "IDLING", "Lorg/pytgcalls/ntgcalls/media/StreamStatus;");

    jobject result;
    switch (status) {
        case ntgcalls::StreamManager::Status::Active:
            result = env->GetStaticObjectField(streamStatusClass, playingField);
            break;
        case ntgcalls::StreamManager::Status::Paused:
            result = env->GetStaticObjectField(streamStatusClass, pausedField);
            break;
        case ntgcalls::StreamManager::Status::Idling:
            result = env->GetStaticObjectField(streamStatusClass, idlingField);
            break;
    }
    env->DeleteLocalRef(streamStatusClass);
    return result;
}

jobject parseMediaStatus(JNIEnv *env, const ntgcalls::StreamManager::MediaStatus &status) {
    jclass mediaStatusClass = env->FindClass("org/pytgcalls/ntgcalls/media/MediaStatus");
    jmethodID constructor = env->GetMethodID(mediaStatusClass, "<init>", "(Lorg/pytgcalls/ntgcalls/media/StreamStatus;Lorg/pytgcalls/ntgcalls/media/StreamStatus;)V");
    jobject capture = parseStreamStatus(env, status.capture);
    jobject playback = parseStreamStatus(env, status.playback);
    jobject result = env->NewObject(mediaStatusClass, constructor, capture, playback);
    env->DeleteLocalRef(mediaStatusClass);
    env->DeleteLocalRef(capture);
    env->DeleteLocalRef(playback);
    return result;
}

jobject parseMediaStatusMap(JNIEnv *env, const std::map<int64_t, ntgcalls::StreamManager::MediaStatus> &calls) {
    jclass mapClass = env->FindClass("java/util/HashMap");
    jmethodID mapConstructor = env->GetMethodID(mapClass, "<init>", "()V");
    jobject hashMap = env->NewObject(mapClass, mapConstructor);
    jmethodID putMethod = env->GetMethodID(mapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    jclass longClass = env->FindClass("java/lang/Long");
    jmethodID longConstructor = env->GetMethodID(longClass, "<init>", "(J)V");
    for (auto const& [key, val] : calls) {
        jobject longKey = env->NewObject(longClass, longConstructor, static_cast<jlong>(key));
        jobject status = parseMediaStatus(env, val);
        env->CallObjectMethod(hashMap, putMethod, longKey, status);
        env->DeleteLocalRef(longKey);
        env->DeleteLocalRef(status);
    }
    env->DeleteLocalRef(mapClass);
    env->DeleteLocalRef(longClass);
    return hashMap;
}

jobject parseMediaDevices(JNIEnv *env, const ntgcalls::MediaDevices &devices) {
    jclass mediaDevicesClass = env->FindClass("org/pytgcalls/ntgcalls/media/MediaDevices");
    jmethodID constructor = env->GetMethodID(mediaDevicesClass, "<init>", "(Ljava/util/List;Ljava/util/List;)V");
    jobject audio = parseDeviceInfoList(env, devices.audio);
    jobject video = parseDeviceInfoList(env, devices.video);
    jobject result = env->NewObject(mediaDevicesClass, constructor, audio, video);
    env->DeleteLocalRef(mediaDevicesClass);
    env->DeleteLocalRef(audio);
    env->DeleteLocalRef(video);
    return result;
}

jobject parseDeviceInfoList(JNIEnv *env, const std::vector<ntgcalls::DeviceInfo> &devices) {
    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID constructor = env->GetMethodID(arrayListClass, "<init>", "(I)V");
    jobject result = env->NewObject(arrayListClass, constructor, static_cast<jint>(devices.size()));
    jmethodID addMethod = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
    for (const auto &element : devices) {
        auto device = parseDeviceInfo(env, element);
        env->CallBooleanMethod(result, addMethod, device);
        env->DeleteLocalRef(device);
    }
    env->DeleteLocalRef(arrayListClass);
    return result;
}

jobject parseDeviceInfo(JNIEnv *env, const ntgcalls::DeviceInfo& device) {
    jclass deviceInfoClass = env->FindClass("org/pytgcalls/ntgcalls/media/DeviceInfo");
    jmethodID constructor = env->GetMethodID(deviceInfoClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
    jobject result = env->NewObject(deviceInfoClass, constructor, parseJString(env, device.name), parseJString(env, device.metadata));
    env->DeleteLocalRef(deviceInfoClass);
    return result;
}

ntgcalls::StreamManager::Mode parseStreamMode(JNIEnv *env, jobject device) {
    jclass streamDeviceClass = env->FindClass("org/pytgcalls/ntgcalls/media/StreamMode");
    jfieldID captureField = env->GetStaticFieldID(streamDeviceClass, "CAPTURE", "Lorg/pytgcalls/ntgcalls/media/StreamMode;");
    jfieldID playbackField = env->GetStaticFieldID(streamDeviceClass, "PLAYBACK", "Lorg/pytgcalls/ntgcalls/media/StreamMode;");
    jobject playback = env->GetStaticObjectField(streamDeviceClass, playbackField);
    jobject capture = env->GetStaticObjectField(streamDeviceClass, captureField);

    ntgcalls::StreamManager::Mode result = ntgcalls::StreamManager::Mode::Capture;
    if (env->IsSameObject(device, capture)) {
        result = ntgcalls::StreamManager::Mode::Capture;
    } else if (env->IsSameObject(device, playback)) {
        result = ntgcalls::StreamManager::Mode::Playback;
    }

    env->DeleteLocalRef(streamDeviceClass);
    env->DeleteLocalRef(playback);
    env->DeleteLocalRef(capture);
    return result;
}

jobject parseJDevice(JNIEnv *env, ntgcalls::StreamManager::Device device) {
    jclass streamDeviceClass = env->FindClass("org/pytgcalls/ntgcalls/media/StreamDevice");
    jfieldID microphoneField = env->GetStaticFieldID(streamDeviceClass, "MICROPHONE", "Lorg/pytgcalls/ntgcalls/media/StreamDevice;");
    jfieldID speakerField = env->GetStaticFieldID(streamDeviceClass, "SPEAKER", "Lorg/pytgcalls/ntgcalls/media/StreamDevice;");
    jfieldID cameraField = env->GetStaticFieldID(streamDeviceClass, "CAMERA", "Lorg/pytgcalls/ntgcalls/media/StreamDevice;");
    jfieldID screenField = env->GetStaticFieldID(streamDeviceClass, "SCREEN", "Lorg/pytgcalls/ntgcalls/media/StreamDevice;");

    jobject result;
    switch (device) {
        case ntgcalls::StreamManager::Device::Microphone:
            result = env->GetStaticObjectField(streamDeviceClass, microphoneField);
            break;
        case ntgcalls::StreamManager::Device::Speaker:
            result = env->GetStaticObjectField(streamDeviceClass, speakerField);
            break;
        case ntgcalls::StreamManager::Device::Camera:
            result = env->GetStaticObjectField(streamDeviceClass, cameraField);
            break;
        case ntgcalls::StreamManager::Device::Screen:
            result = env->GetStaticObjectField(streamDeviceClass, screenField);
            break;
    }
    env->DeleteLocalRef(streamDeviceClass);
    return result;
}

void throwJavaException(JNIEnv *env, std::string name, const std::string& message) {
    if (name == "RuntimeException") {
        name = "java/lang/" + name;
    } else if (name == "FileNotFoundException") {
        name = "java/io/" + name;
    } else {
        std::string from = "Error";
        size_t start_pos = name.find(from);
        if (start_pos != std::string::npos) {
            name.replace(start_pos, from.length(), "");
        }
        name = "org/pytgcalls/ntgcalls/exceptions/" + name + "Exception";
    }
    jclass exceptionClass = env->FindClass(name.c_str());
    if (exceptionClass != nullptr) {
        env->ThrowNew(exceptionClass, message.c_str());
        env->DeleteLocalRef(exceptionClass);
    }
}
