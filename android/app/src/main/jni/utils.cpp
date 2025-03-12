#include "utils.hpp"
#include <sdk/android/native_api/jni/class_loader.h>

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

ntgcalls::JavaVideoCapturerModule* getInstanceVideoCapture(JNIEnv *env, jobject obj) {
    auto ptr = getInstancePtr(env, obj);
    if (ptr != 0) {
        return reinterpret_cast<ntgcalls::JavaVideoCapturerModule*>(ptr);
    }
    return nullptr;
}

jlong getInstancePtr(JNIEnv *env, jobject obj) {
    webrtc::ScopedJavaLocalRef<jclass> clazz{env, env->GetObjectClass(obj)};
    jlong ptr = env->GetLongField(obj,  env->GetFieldID(clazz.obj(), "nativePointer", "J"));
    return ptr;
}

ntgcalls::AudioDescription parseAudioDescription(JNIEnv *env, jobject audioDescription) {
    webrtc::ScopedJavaLocalRef<jclass> audioDescriptionClass{env, env->GetObjectClass(audioDescription)};
    jfieldID inputField = env->GetFieldID(audioDescriptionClass.obj(), "input", "Ljava/lang/String;");
    jfieldID mediaSourceField = env->GetFieldID(audioDescriptionClass.obj(), "mediaSource", "I");
    jfieldID sampleRateField = env->GetFieldID(audioDescriptionClass.obj(), "sampleRate", "I");
    jfieldID channelCountField = env->GetFieldID(audioDescriptionClass.obj(), "channelCount", "I");

    webrtc::ScopedJavaLocalRef<jstring> input{env, reinterpret_cast<jstring>(env->GetObjectField(audioDescription, inputField))};
    auto mediaSource = env->GetIntField(audioDescription, mediaSourceField);
    auto sampleRate = static_cast<uint32_t>(env->GetIntField(audioDescription, sampleRateField));
    auto channelCount = static_cast<uint8_t>(env->GetIntField(audioDescription, channelCountField));

    return {
        parseMediaSource(mediaSource),
        sampleRate,
        channelCount,
        parseString(env, input.obj())
    };
}

ntgcalls::VideoDescription parseVideoDescription(JNIEnv *env, jobject videoDescription) {
    webrtc::ScopedJavaLocalRef<jclass> videoDescriptionClass{env, env->GetObjectClass(videoDescription)};
    jfieldID inputField = env->GetFieldID(videoDescriptionClass.obj(), "input", "Ljava/lang/String;");
    jfieldID mediaSourceField = env->GetFieldID(videoDescriptionClass.obj(), "mediaSource", "I");
    jfieldID widthField = env->GetFieldID(videoDescriptionClass.obj(), "width", "I");
    jfieldID heightField = env->GetFieldID(videoDescriptionClass.obj(), "height", "I");
    jfieldID fpsField = env->GetFieldID(videoDescriptionClass.obj(), "fps", "I");

    webrtc::ScopedJavaLocalRef<jstring> input{env, reinterpret_cast<jstring>(env->GetObjectField(videoDescription, inputField))};
    auto mediaSource = env->GetIntField(videoDescription, mediaSourceField);
    auto width = static_cast<int16_t>(env->GetIntField(videoDescription, widthField));
    auto height = static_cast<int16_t>(env->GetIntField(videoDescription, heightField));
    auto fps = static_cast<uint8_t>(env->GetIntField(videoDescription, fpsField));

    return {
        parseMediaSource(mediaSource),
        width,
        height,
        fps,
        parseString(env, input.obj())
    };
}

ntgcalls::MediaDescription parseMediaDescription(JNIEnv *env, jobject mediaDescription) {
    if (mediaDescription == nullptr) {
        return ntgcalls::MediaDescription();
    }
    webrtc::ScopedJavaLocalRef<jclass> mediaDescriptionClass{env, env->GetObjectClass(mediaDescription)};
    jfieldID micField = env->GetFieldID(mediaDescriptionClass.obj(), "microphone", "Lio/github/pytgcalls/media/AudioDescription;");
    jfieldID speakerField = env->GetFieldID(mediaDescriptionClass.obj(), "speaker", "Lio/github/pytgcalls/media/AudioDescription;");
    jfieldID cameraField = env->GetFieldID(mediaDescriptionClass.obj(), "camera", "Lio/github/pytgcalls/media/VideoDescription;");
    jfieldID screenField = env->GetFieldID(mediaDescriptionClass.obj(), "screen", "Lio/github/pytgcalls/media/VideoDescription;");

    webrtc::ScopedJavaLocalRef<jobject> microphone{env, env->GetObjectField(mediaDescription, micField)};
    webrtc::ScopedJavaLocalRef<jobject> speaker{env, env->GetObjectField(mediaDescription, speakerField)};
    webrtc::ScopedJavaLocalRef<jobject> camera{env, env->GetObjectField(mediaDescription, cameraField)};
    webrtc::ScopedJavaLocalRef<jobject> screen{env, env->GetObjectField(mediaDescription, screenField)};

    return ntgcalls::MediaDescription(
        microphone.obj() != nullptr ? std::optional(parseAudioDescription(env, microphone.obj())) : std::nullopt,
        speaker.obj() != nullptr ? std::optional(parseAudioDescription(env, speaker.obj())) : std::nullopt,
        camera.obj() != nullptr ? std::optional(parseVideoDescription(env, camera.obj())) : std::nullopt,
        screen.obj() != nullptr ? std::optional(parseVideoDescription(env, screen.obj())) : std::nullopt
    );
}

ntgcalls::BaseMediaDescription::MediaSource parseMediaSource(jint mediaSource) {
    ntgcalls::BaseMediaDescription::MediaSource res = ntgcalls::BaseMediaDescription::MediaSource::Unknown;
    if (auto check = ntgcalls::BaseMediaDescription::MediaSource::File;mediaSource == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::MediaSource::Shell;mediaSource == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::MediaSource::FFmpeg;mediaSource == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::MediaSource::Device;mediaSource == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::MediaSource::Desktop;mediaSource == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::MediaSource::External;mediaSource == check) {
        res |= check;
    }
    return res;
}

ntgcalls::DhConfig parseDhConfig(JNIEnv *env, jobject dhConfig) {
    if (dhConfig == nullptr) {
        throw ntgcalls::InvalidParams("DHConfig is required");
    }
    webrtc::ScopedJavaLocalRef<jclass> dhConfigClass{env, env->GetObjectClass(dhConfig)};
    jfieldID gFieldID = env->GetFieldID(dhConfigClass.obj(), "g", "I");
    jfieldID pFieldID = env->GetFieldID(dhConfigClass.obj(), "p", "[B");
    jfieldID randomFieldID = env->GetFieldID(dhConfigClass.obj(), "random", "[B");

    auto g = static_cast<int32_t>(env->GetIntField(dhConfig, gFieldID));
    webrtc::ScopedJavaLocalRef<jbyteArray> pArray{env, reinterpret_cast<jbyteArray>(env->GetObjectField(dhConfig, pFieldID))};
    webrtc::ScopedJavaLocalRef<jbyteArray> randomArray{env, reinterpret_cast<jbyteArray>(env->GetObjectField(dhConfig, randomFieldID))};

    return {
        g,
        parseByteArray(env, pArray.obj()),
        parseByteArray(env, randomArray.obj())
    };
}

std::string parseString(JNIEnv *env, jstring string) {
    if (string == nullptr) {
        return {};
    }
    auto utfChars = env->GetStringUTFChars(string, nullptr);
    auto result = std::string(utfChars);
    env->ReleaseStringUTFChars(string, utfChars);
    return result;
}

webrtc::ScopedJavaLocalRef<jstring> parseJString(JNIEnv *env, const std::string &string) {
    return webrtc::ScopedJavaLocalRef<jstring>{env, env->NewStringUTF(string.c_str())};
}

bytes::vector parseByteArray(JNIEnv *env, jbyteArray byteArray) {
    if (byteArray == nullptr) {
        return {};
    }
    jsize length = env->GetArrayLength(byteArray);
    auto byteBuffer = (uint8_t *) env->GetByteArrayElements(byteArray, nullptr);
    bytes::vector result(length);
    memcpy(&result[0], byteBuffer, length);
    env->ReleaseByteArrayElements(byteArray, (jbyte *) byteBuffer, JNI_ABORT);
    return result;
}

webrtc::ScopedJavaLocalRef<jbyteArray> parseJByteArray(JNIEnv *env, const bytes::vector& byteArray) {
    webrtc::ScopedJavaLocalRef<jbyteArray> result{env, env->NewByteArray(static_cast<jsize>(byteArray.size()))};
    env->SetByteArrayRegion(result.obj(), 0,  static_cast<jsize>(byteArray.size()), reinterpret_cast<const jbyte*>(byteArray.data()));
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

webrtc::ScopedJavaLocalRef<jbyteArray> parseJBinary(JNIEnv *env, const bytes::binary& binary) {
    webrtc::ScopedJavaLocalRef<jbyteArray> result{env, env->NewByteArray(static_cast<jsize>(binary.size()))};
    env->SetByteArrayRegion(result.obj(), 0, static_cast<jsize>(binary.size()), reinterpret_cast<const jbyte*>(binary.data()));
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

webrtc::ScopedJavaLocalRef<jobject> parseAuthParams(JNIEnv *env, const ntgcalls::AuthParams& authParams) {
    const webrtc::ScopedJavaLocalRef<jclass> authParamsClass = webrtc::GetClass(env, "io/github/pytgcalls/p2p/AuthParams");
    jmethodID constructor = env->GetMethodID(authParamsClass.obj(), "<init>", "([BJ)V");
    auto g_a_or_b = parseJByteArray(env, authParams.g_a_or_b);
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(authParamsClass.obj(), constructor, g_a_or_b.obj(), authParams.key_fingerprint)};
}

std::vector<std::string> parseStringList(JNIEnv *env, jobject list) {
    if (list == nullptr) {
        return {};
    }
    const webrtc::ScopedJavaLocalRef<jclass> listClass{env, env->GetObjectClass(list)};
    jmethodID sizeMethod = env->GetMethodID(listClass.obj(), "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass.obj(), "get", "(I)Ljava/lang/Object;");
    std::vector<std::string> result;
    jint size = env->CallIntMethod(list, sizeMethod);
    for (int i = 0; i < size; i++) {
        const webrtc::ScopedJavaLocalRef<jstring> element{env, reinterpret_cast<jstring>(env->CallObjectMethod(list, getMethod, i))};
        result.push_back(parseString(env, element.obj()));
    }
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseJStringList(JNIEnv *env, const std::vector<std::string> &list) {
    const webrtc::ScopedJavaLocalRef<jclass> arrayListClass = webrtc::GetClass(env, "java/util/ArrayList");
    jmethodID constructor = env->GetMethodID(arrayListClass.obj(), "<init>", "(I)V");
    const webrtc::ScopedJavaLocalRef<jobject> result{env, env->NewObject(arrayListClass.obj(), constructor, static_cast<jint>(list.size()))};
    jmethodID addMethod = env->GetMethodID(arrayListClass.obj(), "add", "(Ljava/lang/Object;)Z");
    for (const auto &element : list) {
        auto string = parseJString(env, element);
        env->CallBooleanMethod(result.obj(), addMethod, string.obj());
    }
    return result;
}

ntgcalls::RTCServer parseRTCServer(JNIEnv *env, jobject rtcServer) {
    const webrtc::ScopedJavaLocalRef<jclass> dhRTCServerClass{env, env->GetObjectClass(rtcServer)};
    jfieldID idFieldID = env->GetFieldID(dhRTCServerClass.obj(), "id", "J");
    jfieldID ipv4FieldID = env->GetFieldID(dhRTCServerClass.obj(), "ipv4", "Ljava/lang/String;");
    jfieldID ipv6FieldID = env->GetFieldID(dhRTCServerClass.obj(), "ipv6", "Ljava/lang/String;");
    jfieldID portFieldID = env->GetFieldID(dhRTCServerClass.obj(), "port", "I");
    jfieldID usernameFieldID = env->GetFieldID(dhRTCServerClass.obj(), "username", "Ljava/lang/String;");
    jfieldID passwordFieldID = env->GetFieldID(dhRTCServerClass.obj(), "password", "Ljava/lang/String;");
    jfieldID turnFieldID = env->GetFieldID(dhRTCServerClass.obj(), "turn", "Z");
    jfieldID stunFieldID = env->GetFieldID(dhRTCServerClass.obj(), "stun", "Z");
    jfieldID tcpFieldID = env->GetFieldID(dhRTCServerClass.obj(), "tcp", "Z");
    jfieldID peerTagFieldID = env->GetFieldID(dhRTCServerClass.obj(), "peerTag", "[B");

    const auto id = static_cast<uint64_t>(env->GetLongField(rtcServer, idFieldID));
    const webrtc::ScopedJavaLocalRef<jstring> ipv4{env, reinterpret_cast<jstring>(env->GetObjectField(rtcServer, ipv4FieldID))};
    const webrtc::ScopedJavaLocalRef<jstring> ipv6{env, reinterpret_cast<jstring>(env->GetObjectField(rtcServer, ipv6FieldID))};
    const auto port = static_cast<uint16_t>(env->GetIntField(rtcServer, portFieldID));
    const webrtc::ScopedJavaLocalRef<jstring> username{env, reinterpret_cast<jstring>(env->GetObjectField(rtcServer, usernameFieldID))};
    const webrtc::ScopedJavaLocalRef<jstring> password{env, reinterpret_cast<jstring>(env->GetObjectField(rtcServer, passwordFieldID))};
    const auto turn = static_cast<bool>(env->GetBooleanField(rtcServer, turnFieldID));
    const auto stun = static_cast<bool>(env->GetBooleanField(rtcServer, stunFieldID));
    const auto tcp = static_cast<bool>(env->GetBooleanField(rtcServer, tcpFieldID));
    const webrtc::ScopedJavaLocalRef<jbyteArray> peerTag{env, reinterpret_cast<jbyteArray>(env->GetObjectField(rtcServer, peerTagFieldID))};

    return {
        id,
        parseString(env, ipv4.obj()),
        parseString(env, ipv6.obj()),
        port,
        username.obj() != nullptr ? std::optional(parseString(env, username.obj())) : std::nullopt,
        password.obj() != nullptr ? std::optional(parseString(env, password.obj())) : std::nullopt,
        turn,
        stun,
        tcp,
        peerTag.obj() != nullptr ? std::optional(parseBinary(env, peerTag.obj())) : std::nullopt
    };
}

std::vector<ntgcalls::RTCServer> parseRTCServerList(JNIEnv *env, jobject list) {
    if (list == nullptr) {
        return {};
    }
    const webrtc::ScopedJavaLocalRef<jclass> listClass{env, env->GetObjectClass(list)};
    jmethodID sizeMethod = env->GetMethodID(listClass.obj(), "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass.obj(), "get", "(I)Ljava/lang/Object;");
    std::vector<ntgcalls::RTCServer> result;
    for (int i = 0; i < env->CallIntMethod(list, sizeMethod); i++) {
        const webrtc::ScopedJavaLocalRef<jobject> element{env, env->CallObjectMethod(list, getMethod, i)};
        result.push_back(parseRTCServer(env, element.obj()));
    }
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseJMediaState(JNIEnv *env, ntgcalls::MediaState mediaState) {
    const webrtc::ScopedJavaLocalRef<jclass> mediaStateClass = webrtc::GetClass(env, "io/github/pytgcalls/media/MediaState");
    jmethodID constructor = env->GetMethodID(mediaStateClass.obj(), "<init>", "(ZZZ)V");
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(mediaStateClass.obj(), constructor, mediaState.muted, mediaState.videoPaused, mediaState.videoStopped)};
}

webrtc::ScopedJavaLocalRef<jobject> parseJProtocol(JNIEnv *env, const ntgcalls::Protocol &protocol) {
    const webrtc::ScopedJavaLocalRef<jclass> protocolClass = webrtc::GetClass(env, "io/github/pytgcalls/p2p/Protocol");
    jmethodID constructor = env->GetMethodID(protocolClass.obj(), "<init>", "(IIZZLjava/util/List;)V");
    auto libraryVersions = parseJStringList(env, protocol.library_versions);
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(protocolClass.obj(), constructor, protocol.min_layer, protocol.max_layer, protocol.udp_p2p, protocol.udp_reflector, libraryVersions.obj())};
}

webrtc::ScopedJavaLocalRef<jobject> parseJStreamType(JNIEnv *env, ntgcalls::StreamManager::Type type) {
    const webrtc::ScopedJavaLocalRef<jclass> streamTypeClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamType");
    jfieldID audioField = env->GetStaticFieldID(streamTypeClass.obj(), "AUDIO", "Lio/github/pytgcalls/media/StreamType;");
    jfieldID videoField = env->GetStaticFieldID(streamTypeClass.obj(), "VIDEO", "Lio/github/pytgcalls/media/StreamType;");

    switch (type) {
        case ntgcalls::StreamManager::Type::Audio:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamTypeClass.obj(), audioField)};
        case ntgcalls::StreamManager::Type::Video:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamTypeClass.obj(), videoField)};
    }
    return nullptr;
}

webrtc::ScopedJavaLocalRef<jobject> parseJConnectionState(JNIEnv *env, ntgcalls::NetworkInfo::ConnectionState state) {
    const webrtc::ScopedJavaLocalRef<jclass> connectionStateClass = webrtc::GetClass(env, "io/github/pytgcalls/NetworkInfo$State");
    jfieldID connectingField = env->GetStaticFieldID(connectionStateClass.obj(), "CONNECTING", "Lio/github/pytgcalls/NetworkInfo$State;");
    jfieldID connectedField = env->GetStaticFieldID(connectionStateClass.obj(), "CONNECTED", "Lio/github/pytgcalls/NetworkInfo$State;");
    jfieldID failedField = env->GetStaticFieldID(connectionStateClass.obj(), "FAILED", "Lio/github/pytgcalls/NetworkInfo$State;");
    jfieldID timeoutField = env->GetStaticFieldID(connectionStateClass.obj(), "TIMEOUT", "Lio/github/pytgcalls/NetworkInfo$State;");
    jfieldID closedField = env->GetStaticFieldID(connectionStateClass.obj(), "CLOSED", "Lio/github/pytgcalls/NetworkInfo$State;");

    switch (state) {
        case ntgcalls::NetworkInfo::ConnectionState::Connecting:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(connectionStateClass.obj(), connectingField)};
        case ntgcalls::NetworkInfo::ConnectionState::Connected:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(connectionStateClass.obj(), connectedField)};
        case ntgcalls::NetworkInfo::ConnectionState::Failed:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(connectionStateClass.obj(), failedField)};
        case ntgcalls::NetworkInfo::ConnectionState::Timeout:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(connectionStateClass.obj(), timeoutField)};
        case ntgcalls::NetworkInfo::ConnectionState::Closed:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(connectionStateClass.obj(), closedField)};
    }
    return nullptr;
}

webrtc::ScopedJavaLocalRef<jobject> parseJNetworkInfo(JNIEnv *env, ntgcalls::NetworkInfo state) {
    const webrtc::ScopedJavaLocalRef<jclass> networkInfoClass = webrtc::GetClass(env, "io/github/pytgcalls/NetworkInfo");
    jmethodID constructor = env->GetMethodID(networkInfoClass.obj(), "<init>", "(Lio/github/pytgcalls/NetworkInfo$Kind;Lio/github/pytgcalls/NetworkInfo$State;)V");
    auto kind = parseJNetworkInfoKind(env, state.kind);
    auto connectionState = parseJConnectionState(env, state.connectionState);
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(networkInfoClass.obj(), constructor, kind.obj(), connectionState.obj())};
}

webrtc::ScopedJavaLocalRef<jobject> parseJNetworkInfoKind(JNIEnv *env, ntgcalls::NetworkInfo::Kind kind) {
    const webrtc::ScopedJavaLocalRef<jclass> kindClass = webrtc::GetClass(env, "io/github/pytgcalls/NetworkInfo$Kind");
    jfieldID normalField = env->GetStaticFieldID(kindClass.obj(), "NORMAL", "Lio/github/pytgcalls/NetworkInfo$Kind;");
    jfieldID presentationField = env->GetStaticFieldID(kindClass.obj(), "PRESENTATION", "Lio/github/pytgcalls/NetworkInfo$Kind;");

    switch (kind) {
        case ntgcalls::NetworkInfo::Kind::Normal:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(kindClass.obj(), normalField)};
        case ntgcalls::NetworkInfo::Kind::Presentation:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(kindClass.obj(), presentationField)};
    }
    return nullptr;
}

webrtc::ScopedJavaLocalRef<jobject> parseJStreamStatus(JNIEnv *env, ntgcalls::StreamManager::Status status) {
    const webrtc::ScopedJavaLocalRef<jclass> streamStatusClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamStatus");
    jfieldID playingField = env->GetStaticFieldID(streamStatusClass.obj(), "ACTIVE", "Lio/github/pytgcalls/media/StreamStatus;");
    jfieldID pausedField = env->GetStaticFieldID(streamStatusClass.obj(), "PAUSED", "Lio/github/pytgcalls/media/StreamStatus;");
    jfieldID idlingField = env->GetStaticFieldID(streamStatusClass.obj(), "IDLING", "Lio/github/pytgcalls/media/StreamStatus;");

    switch (status) {
        case ntgcalls::StreamManager::Status::Active:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamStatusClass.obj(), playingField)};
        case ntgcalls::StreamManager::Status::Paused:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamStatusClass.obj(), pausedField)};
        case ntgcalls::StreamManager::Status::Idling:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamStatusClass.obj(), idlingField)};
    }
    return nullptr;
}

webrtc::ScopedJavaLocalRef<jobject> parseJCallInfo(JNIEnv *env, const ntgcalls::StreamManager::CallInfo &status) {
    const webrtc::ScopedJavaLocalRef<jclass> callInfoClass = webrtc::GetClass(env, "io/github/pytgcalls/media/CallInfo");

    jmethodID constructor = env->GetMethodID(callInfoClass.obj(), "<init>", "(Lio/github/pytgcalls/media/StreamStatus;Lio/github/pytgcalls/media/StreamStatus;)V");
    auto capture = parseJStreamStatus(env, status.capture);
    auto playback = parseJStreamStatus(env, status.playback);
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(callInfoClass.obj(), constructor, capture.obj(), playback.obj())};
}

webrtc::ScopedJavaLocalRef<jobject> parseJCallInfoMap(JNIEnv *env, const std::map<int64_t, ntgcalls::StreamManager::CallInfo> &calls) {
    const webrtc::ScopedJavaLocalRef<jclass> mapClass = webrtc::GetClass(env, "java/util/HashMap");
    jmethodID mapConstructor = env->GetMethodID(mapClass.obj(), "<init>", "()V");
    webrtc::ScopedJavaLocalRef<jobject> hashMap{env, env->NewObject(mapClass.obj(), mapConstructor)};
    jmethodID putMethod = env->GetMethodID(mapClass.obj(), "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    const webrtc::ScopedJavaLocalRef<jclass> longClass = webrtc::GetClass(env, "java/lang/Long");
    jmethodID longConstructor = env->GetMethodID(longClass.obj(), "<init>", "(J)V");
    for (auto const& [key, val] : calls) {
        webrtc::ScopedJavaLocalRef<jobject> longKey{env, env->NewObject(longClass.obj(), longConstructor, static_cast<jlong>(key))};
        auto status = parseJCallInfo(env, val);
        env->CallObjectMethod(hashMap.obj(), putMethod, longKey.obj(), status.obj());
    }
    return hashMap;
}

webrtc::ScopedJavaLocalRef<jobject> parseJMediaDevices(JNIEnv *env, const ntgcalls::MediaDevices &devices) {
    const webrtc::ScopedJavaLocalRef<jclass> mediaDevicesClass = webrtc::GetClass(env, "io/github/pytgcalls/media/MediaDevices");
    jmethodID constructor = env->GetMethodID(mediaDevicesClass.obj(), "<init>", "(Ljava/util/List;Ljava/util/List;Ljava/util/List;Ljava/util/List;)V");
    auto microphone = parseJDeviceInfoList(env, devices.microphone);
    auto speaker = parseJDeviceInfoList(env, devices.speaker);
    auto camera = parseJDeviceInfoList(env, devices.camera);
    auto screen = parseJDeviceInfoList(env, devices.screen);
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(mediaDevicesClass.obj(), constructor, microphone.obj(), speaker.obj(), camera.obj(), screen.obj())};
}

webrtc::ScopedJavaLocalRef<jobject> parseJDeviceInfoList(JNIEnv *env, const std::vector<ntgcalls::DeviceInfo> &devices) {
    const webrtc::ScopedJavaLocalRef<jclass> arrayListClass = webrtc::GetClass(env, "java/util/ArrayList");
    jmethodID constructor = env->GetMethodID(arrayListClass.obj(), "<init>", "(I)V");
    const webrtc::ScopedJavaLocalRef<jobject> result{env, env->NewObject(arrayListClass.obj(), constructor, static_cast<jint>(devices.size()))};
    jmethodID addMethod = env->GetMethodID(arrayListClass.obj(), "add", "(Ljava/lang/Object;)Z");
    for (const auto &element : devices) {
        auto device = parseJDeviceInfo(env, element);
        env->CallBooleanMethod(result.obj(), addMethod, device.obj());
    }
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseJDeviceInfo(JNIEnv *env, const ntgcalls::DeviceInfo& device) {
    const webrtc::ScopedJavaLocalRef<jclass> deviceInfoClass = webrtc::GetClass(env, "io/github/pytgcalls/media/DeviceInfo");
    jmethodID constructor = env->GetMethodID(deviceInfoClass.obj(), "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
    auto deviceName = parseJString(env, device.name);
    auto metadata = parseJString(env, device.metadata);
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(deviceInfoClass.obj(), constructor, deviceName.obj(), metadata.obj())};
}

ntgcalls::StreamManager::Mode parseStreamMode(JNIEnv *env, jobject device) {
    const webrtc::ScopedJavaLocalRef<jclass> streamModeClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamMode");
    jfieldID captureField = env->GetStaticFieldID(streamModeClass.obj(), "CAPTURE", "Lio/github/pytgcalls/media/StreamMode;");
    jfieldID playbackField = env->GetStaticFieldID(streamModeClass.obj(), "PLAYBACK", "Lio/github/pytgcalls/media/StreamMode;");
    const webrtc::ScopedJavaLocalRef<jobject> playback{env, env->GetStaticObjectField(streamModeClass.obj(), playbackField)};
    const webrtc::ScopedJavaLocalRef<jobject> capture{env, env->GetStaticObjectField(streamModeClass.obj(), captureField)};

    if (env->IsSameObject(device, capture.obj())) {
        return ntgcalls::StreamManager::Mode::Capture;
    } else if (env->IsSameObject(device, playback.obj())) {
        return ntgcalls::StreamManager::Mode::Playback;
    }
    return ntgcalls::StreamManager::Mode::Capture;
}

webrtc::ScopedJavaLocalRef<jobject> parseJStreamMode(JNIEnv *env, ntgcalls::StreamManager::Mode mode) {
    const webrtc::ScopedJavaLocalRef<jclass> streamModeClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamMode");
    jfieldID captureField = env->GetStaticFieldID(streamModeClass.obj(), "CAPTURE", "Lio/github/pytgcalls/media/StreamMode;");
    jfieldID playbackField = env->GetStaticFieldID(streamModeClass.obj(), "PLAYBACK", "Lio/github/pytgcalls/media/StreamMode;");
    switch (mode) {
        case ntgcalls::StreamManager::Mode::Capture:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamModeClass.obj(), captureField)};
        case ntgcalls::StreamManager::Mode::Playback:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamModeClass.obj(), playbackField)};
    }
    return nullptr;
}

ntgcalls::StreamManager::Device parseDevice(JNIEnv *env, jobject device) {
    const webrtc::ScopedJavaLocalRef<jclass> streamDeviceClass = webrtc::GetClass(env,"io/github/pytgcalls/media/StreamDevice");
    jfieldID microphoneField = env->GetStaticFieldID(streamDeviceClass.obj(), "MICROPHONE", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID speakerField = env->GetStaticFieldID(streamDeviceClass.obj(), "SPEAKER", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID cameraField = env->GetStaticFieldID(streamDeviceClass.obj(), "CAMERA", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID screenField = env->GetStaticFieldID(streamDeviceClass.obj(), "SCREEN", "Lio/github/pytgcalls/media/StreamDevice;");
    const webrtc::ScopedJavaLocalRef<jobject> microphone{env, env->GetStaticObjectField(streamDeviceClass.obj(), microphoneField)};
    const webrtc::ScopedJavaLocalRef<jobject> speaker{env, env->GetStaticObjectField(streamDeviceClass.obj(), speakerField)};
    const webrtc::ScopedJavaLocalRef<jobject> camera{env, env->GetStaticObjectField(streamDeviceClass.obj(), cameraField)};
    const webrtc::ScopedJavaLocalRef<jobject> screen{env, env->GetStaticObjectField(streamDeviceClass.obj(), screenField)};

    if (env->IsSameObject(device, microphone.obj())) {
        return ntgcalls::StreamManager::Device::Microphone;
    } else if (env->IsSameObject(device, speaker.obj())) {
        return ntgcalls::StreamManager::Device::Speaker;
    } else if (env->IsSameObject(device, camera.obj())) {
        return ntgcalls::StreamManager::Device::Camera;
    } else if (env->IsSameObject(device, screen.obj())) {
        return ntgcalls::StreamManager::Device::Screen;
    }
    return ntgcalls::StreamManager::Device::Microphone;
}

webrtc::ScopedJavaLocalRef<jobject> parseJDevice(JNIEnv *env, ntgcalls::StreamManager::Device device) {
    const webrtc::ScopedJavaLocalRef<jclass> streamDeviceClass = webrtc::GetClass(env,"io/github/pytgcalls/media/StreamDevice");
    jfieldID microphoneField = env->GetStaticFieldID(streamDeviceClass.obj(), "MICROPHONE", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID speakerField = env->GetStaticFieldID(streamDeviceClass.obj(), "SPEAKER", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID cameraField = env->GetStaticFieldID(streamDeviceClass.obj(), "CAMERA", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID screenField = env->GetStaticFieldID(streamDeviceClass.obj(), "SCREEN", "Lio/github/pytgcalls/media/StreamDevice;");

    switch (device) {
        case ntgcalls::StreamManager::Device::Microphone:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamDeviceClass.obj(), microphoneField)};
        case ntgcalls::StreamManager::Device::Speaker:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamDeviceClass.obj(), speakerField)};
        case ntgcalls::StreamManager::Device::Camera:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamDeviceClass.obj(), cameraField)};
        case ntgcalls::StreamManager::Device::Screen:
            return webrtc::ScopedJavaLocalRef<jobject>{env, env->GetStaticObjectField(streamDeviceClass.obj(), screenField)};
    }
    return nullptr;
}

wrtc::FrameData parseFrameData(JNIEnv *env, jobject frameData) {
    const webrtc::ScopedJavaLocalRef<jclass> frameDataClass{env, env->GetObjectClass(frameData)};
    jfieldID absoluteCaptureTimestampMsField = env->GetFieldID(frameDataClass.obj(), "absoluteCaptureTimestampMs", "J");
    jfieldID widthField = env->GetFieldID(frameDataClass.obj(), "width", "I");
    jfieldID heightField = env->GetFieldID(frameDataClass.obj(), "height", "I");
    jfieldID rotationField = env->GetFieldID(frameDataClass.obj(), "rotation", "I");
    const auto absoluteCaptureTimestampMs = static_cast<int64_t>(env->GetLongField(frameData, absoluteCaptureTimestampMsField));
    const auto width = static_cast<uint16_t>(env->GetIntField(frameData, widthField));
    const auto height = static_cast<uint16_t>(env->GetIntField(frameData, heightField));
    const auto rotation = static_cast<uint8_t>(env->GetIntField(frameData, rotationField));
    return {absoluteCaptureTimestampMs, (webrtc::VideoRotation) rotation, width, height};
}

webrtc::ScopedJavaLocalRef<jobject> parseJFrameData(JNIEnv *env, const wrtc::FrameData& frameData) {
    const webrtc::ScopedJavaLocalRef<jclass> frameDataClass = webrtc::GetClass(env,"io/github/pytgcalls/media/FrameData");
    jmethodID constructor = env->GetMethodID(frameDataClass.obj(), "<init>", "(JIII)V");
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(frameDataClass.obj(), constructor, frameData.absoluteCaptureTimestampMs, static_cast<jint>(frameData.width), static_cast<jint>(frameData.height), static_cast<jint>(frameData.rotation))};
}

webrtc::ScopedJavaLocalRef<jobject> parseJRemoteSource(JNIEnv *env, const ntgcalls::RemoteSource& source) {
    const webrtc::ScopedJavaLocalRef<jclass> remoteSourceClass = webrtc::GetClass(env,"io/github/pytgcalls/media/RemoteSource");
    jmethodID constructor = env->GetMethodID(remoteSourceClass.obj(), "<init>", "(ILio/github/pytgcalls/media/StreamStatus;Lio/github/pytgcalls/media/StreamDevice;)V");
    const webrtc::ScopedJavaLocalRef<jclass> sourceStateClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamStatus");
    webrtc::ScopedJavaLocalRef<jobject> state = parseJStreamStatus(env, source.state);
    auto device = parseJDevice(env, source.device);
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(remoteSourceClass.obj(), constructor, source.ssrc, state.obj(), device.obj())};
}

wrtc::SsrcGroup parseSsrcGroup(JNIEnv *env, jobject ssrcGroup) {
    const webrtc::ScopedJavaLocalRef<jclass> ssrcGroupClass{env, env->GetObjectClass(ssrcGroup)};
    jfieldID semanticsField = env->GetFieldID(ssrcGroupClass.obj(), "semantics", "Ljava/lang/String;");
    jfieldID ssrcGroupsField = env->GetFieldID(ssrcGroupClass.obj(), "ssrcGroups", "Ljava/util/List;");

    webrtc::ScopedJavaLocalRef<jstring> semantics{env, reinterpret_cast<jstring>(env->GetObjectField(ssrcGroup, semanticsField))};
    webrtc::ScopedJavaLocalRef<jobject> ssrcGroups{env, env->GetObjectField(ssrcGroup, ssrcGroupsField)};
    const webrtc::ScopedJavaLocalRef<jclass> listClass{env, env->GetObjectClass(ssrcGroups.obj())};
    jmethodID sizeMethod = env->GetMethodID(listClass.obj(), "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass.obj(), "get", "(I)Ljava/lang/Object;");
    std::vector<uint32_t> result;
    int size = env->CallIntMethod(ssrcGroups.obj(), sizeMethod);
    result.reserve(size);
    for (int i = 0; i < size; i++) {
        result.push_back(static_cast<uint32_t>(env->CallIntMethod(ssrcGroups.obj(), getMethod, i)));
    }
    return {parseString(env, semantics.obj()), result};
}

std::vector<wrtc::SsrcGroup> parseSsrcGroupList(JNIEnv *env, jobject list) {
    if (list == nullptr) {
        return {};
    }
    const webrtc::ScopedJavaLocalRef<jclass> listClass{env, env->GetObjectClass(list)};
    jmethodID sizeMethod = env->GetMethodID(listClass.obj(), "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass.obj(), "get", "(I)Ljava/lang/Object;");
    std::vector<wrtc::SsrcGroup> result;
    for (int i = 0; i < env->CallIntMethod(list, sizeMethod); i++) {
        const webrtc::ScopedJavaLocalRef<jobject> element{env, env->CallObjectMethod(list, getMethod, i)};
        result.push_back(parseSsrcGroup(env, element.obj()));
    }
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseJFrame(JNIEnv *env, const wrtc::Frame& frame) {
    const webrtc::ScopedJavaLocalRef<jclass> frameClass = webrtc::GetClass(env,"io/github/pytgcalls/media/Frame");
    jmethodID constructor = env->GetMethodID(frameClass.obj(), "<init>", "(J[BLio/github/pytgcalls/media/FrameData;)V");
    auto data = parseJBinary(env, frame.data);
    auto frameData = parseJFrameData(env, frame.frameData);
    return webrtc::ScopedJavaLocalRef<jobject>{env, env->NewObject(frameClass.obj(), constructor, frame.ssrc, data.obj(), frameData.obj())};
}

webrtc::ScopedJavaLocalRef<jobject> parseJFrames(JNIEnv *env, const std::vector<wrtc::Frame>& frame) {
    const webrtc::ScopedJavaLocalRef<jclass> arrayListClass = webrtc::GetClass(env, "java/util/ArrayList");
    jmethodID constructor = env->GetMethodID(arrayListClass.obj(), "<init>", "(I)V");
    const webrtc::ScopedJavaLocalRef<jobject> result{env, env->NewObject(arrayListClass.obj(), constructor, static_cast<jint>(frame.size()))};
    jmethodID addMethod = env->GetMethodID(arrayListClass.obj(), "add", "(Ljava/lang/Object;)Z");
    for (const auto &element : frame) {
        auto jFrame = parseJFrame(env, element);
        env->CallBooleanMethod(result.obj(), addMethod, jFrame.obj());
    }
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
        name = "io/github/pytgcalls/exceptions/" + name + "Exception";
    }
    const webrtc::ScopedJavaLocalRef<jclass> exceptionClass = webrtc::GetClass(env, name.c_str());
    if (!exceptionClass.is_null()) {
        env->ThrowNew(exceptionClass.obj(), message.c_str());
    }
}
