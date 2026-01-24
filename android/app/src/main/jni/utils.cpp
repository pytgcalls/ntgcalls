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
    auto ptr = getInstancePtr(env, obj, "io/github/pytgcalls/devices/JavaAudioDeviceModule");
    if (ptr != 0) {
        return reinterpret_cast<ntgcalls::JavaAudioDeviceModule*>(ptr);
    }
    return nullptr;
}

ntgcalls::JavaVideoCapturerModule* getInstanceVideoCapture(JNIEnv *env, jobject obj) {
    auto ptr = getInstancePtr(env, obj, "io/github/pytgcalls/devices/JavaVideoCapturerModule");
    if (ptr != 0) {
        return reinterpret_cast<ntgcalls::JavaVideoCapturerModule*>(ptr);
    }
    return nullptr;
}

jlong getInstancePtr(JNIEnv *env, jobject obj, const std::string& name) {
    const auto clazz = webrtc::GetClass(env, name.c_str());
    return env->GetLongField(obj,  env->GetFieldID(clazz.obj(), "nativePointer", "J"));
}

ntgcalls::AudioDescription parseAudioDescription(JNIEnv *env, jobject audioDescription) {
    const auto audioDescriptionClass = webrtc::GetClass(env, "io/github/pytgcalls/media/AudioDescription");
    jfieldID inputField = env->GetFieldID(audioDescriptionClass.obj(), "input", "Ljava/lang/String;");
    jfieldID mediaSourceField = env->GetFieldID(audioDescriptionClass.obj(), "mediaSource", "Lio/github/pytgcalls/media/MediaSource;");
    jfieldID sampleRateField = env->GetFieldID(audioDescriptionClass.obj(), "sampleRate", "I");
    jfieldID channelCountField = env->GetFieldID(audioDescriptionClass.obj(), "channelCount", "I");
    jfieldID keepOpenField = env->GetFieldID(audioDescriptionClass.obj(), "keepOpen", "Z");

    const auto input = webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, reinterpret_cast<jstring>(env->GetObjectField(audioDescription, inputField)));
    const auto mediaSource = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetObjectField(audioDescription, mediaSourceField));
    const auto sampleRate = static_cast<uint32_t>(env->GetIntField(audioDescription, sampleRateField));
    const auto channelCount = static_cast<uint8_t>(env->GetIntField(audioDescription, channelCountField));
    const auto keepOpen = static_cast<bool>(env->GetBooleanField(audioDescription, keepOpenField));

    return {
        parseMediaSource(env, mediaSource.obj()),
        sampleRate,
        channelCount,
        parseString(env, input.obj()),
        keepOpen
    };
}

ntgcalls::VideoDescription parseVideoDescription(JNIEnv *env, jobject videoDescription) {
    const auto videoDescriptionClass = webrtc::GetClass(env, "io/github/pytgcalls/media/VideoDescription");
    jfieldID inputField = env->GetFieldID(videoDescriptionClass.obj(), "input", "Ljava/lang/String;");
    jfieldID mediaSourceField = env->GetFieldID(videoDescriptionClass.obj(), "mediaSource", "Lio/github/pytgcalls/media/MediaSource;");
    jfieldID widthField = env->GetFieldID(videoDescriptionClass.obj(), "width", "I");
    jfieldID heightField = env->GetFieldID(videoDescriptionClass.obj(), "height", "I");
    jfieldID fpsField = env->GetFieldID(videoDescriptionClass.obj(), "fps", "I");
    jfieldID keepOpenField = env->GetFieldID(videoDescriptionClass.obj(), "keepOpen", "Z");

    const auto input = webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, reinterpret_cast<jstring>(env->GetObjectField(videoDescription, inputField)));
    const auto mediaSource = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetObjectField(videoDescription, mediaSourceField));
    const auto width = static_cast<int16_t>(env->GetIntField(videoDescription, widthField));
    const auto height = static_cast<int16_t>(env->GetIntField(videoDescription, heightField));
    const auto fps = static_cast<uint8_t>(env->GetIntField(videoDescription, fpsField));
    const auto keepOpen = static_cast<bool>(env->GetBooleanField(videoDescription, keepOpenField));

    return {
        parseMediaSource(env, mediaSource.obj()),
        width,
        height,
        fps,
        parseString(env, input.obj()),
        keepOpen
    };
}

ntgcalls::MediaDescription parseMediaDescription(JNIEnv *env, jobject mediaDescription) {
    if (mediaDescription == nullptr) {
        return ntgcalls::MediaDescription();
    }
    const auto mediaDescriptionClass = webrtc::GetClass(env, "io/github/pytgcalls/media/MediaDescription");
    jfieldID micField = env->GetFieldID(mediaDescriptionClass.obj(), "microphone", "Lio/github/pytgcalls/media/AudioDescription;");
    jfieldID speakerField = env->GetFieldID(mediaDescriptionClass.obj(), "speaker", "Lio/github/pytgcalls/media/AudioDescription;");
    jfieldID cameraField = env->GetFieldID(mediaDescriptionClass.obj(), "camera", "Lio/github/pytgcalls/media/VideoDescription;");
    jfieldID screenField = env->GetFieldID(mediaDescriptionClass.obj(), "screen", "Lio/github/pytgcalls/media/VideoDescription;");

    const auto microphone = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetObjectField(mediaDescription, micField));
    const auto speaker = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetObjectField(mediaDescription, speakerField));
    const auto camera = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetObjectField(mediaDescription, cameraField));
    const auto screen = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetObjectField(mediaDescription, screenField));

    return ntgcalls::MediaDescription(
        microphone.obj() != nullptr ? std::optional(parseAudioDescription(env, microphone.obj())) : std::nullopt,
        speaker.obj() != nullptr ? std::optional(parseAudioDescription(env, speaker.obj())) : std::nullopt,
        camera.obj() != nullptr ? std::optional(parseVideoDescription(env, camera.obj())) : std::nullopt,
        screen.obj() != nullptr ? std::optional(parseVideoDescription(env, screen.obj())) : std::nullopt
    );
}

ntgcalls::BaseMediaDescription::MediaSource parseMediaSource(JNIEnv *env, jobject mediaSource) {
    const auto streamModeClass = webrtc::GetClass(env, "io/github/pytgcalls/media/MediaSource");
    jfieldID fileField = env->GetStaticFieldID(streamModeClass.obj(), "FILE", "Lio/github/pytgcalls/media/MediaSource;");
    jfieldID shellField = env->GetStaticFieldID(streamModeClass.obj(), "SHELL", "Lio/github/pytgcalls/media/MediaSource;");
    jfieldID ffmpegField = env->GetStaticFieldID(streamModeClass.obj(), "FFMPEG", "Lio/github/pytgcalls/media/MediaSource;");
    jfieldID deviceField = env->GetStaticFieldID(streamModeClass.obj(), "DEVICE", "Lio/github/pytgcalls/media/MediaSource;");
    jfieldID desktopField = env->GetStaticFieldID(streamModeClass.obj(), "DESKTOP", "Lio/github/pytgcalls/media/MediaSource;");
    jfieldID externalField = env->GetStaticFieldID(streamModeClass.obj(), "EXTERNAL", "Lio/github/pytgcalls/media/MediaSource;");
    const auto file = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), fileField));
    const auto shell = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), shellField));
    const auto ffmpeg = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), ffmpegField));
    const auto device = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), deviceField));
    const auto desktop = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), desktopField));
    const auto external = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), externalField));

    if (env->IsSameObject(mediaSource, file.obj())) {
        return ntgcalls::BaseMediaDescription::MediaSource::File;
    } else if (env->IsSameObject(mediaSource, shell.obj())) {
        return ntgcalls::BaseMediaDescription::MediaSource::Shell;
    } else if (env->IsSameObject(mediaSource, ffmpeg.obj())) {
        return ntgcalls::BaseMediaDescription::MediaSource::FFmpeg;
    } else if (env->IsSameObject(mediaSource, device.obj())) {
        return ntgcalls::BaseMediaDescription::MediaSource::Device;
    } else if (env->IsSameObject(mediaSource, desktop.obj())) {
        return ntgcalls::BaseMediaDescription::MediaSource::Desktop;
    } else if (env->IsSameObject(mediaSource, external.obj())) {
        return ntgcalls::BaseMediaDescription::MediaSource::External;
    }
    throw ntgcalls::InvalidParams("Unknown media source");
}

ntgcalls::DhConfig parseDhConfig(JNIEnv *env, jobject dhConfig) {
    if (dhConfig == nullptr) {
        throw ntgcalls::InvalidParams("DHConfig is required");
    }
    const auto dhConfigClass = webrtc::GetClass(env, "io/github/pytgcalls/p2p/DhConfig");
    jfieldID gFieldID = env->GetFieldID(dhConfigClass.obj(), "g", "I");
    jfieldID pFieldID = env->GetFieldID(dhConfigClass.obj(), "p", "[B");
    jfieldID randomFieldID = env->GetFieldID(dhConfigClass.obj(), "random", "[B");

    const auto g = static_cast<int32_t>(env->GetIntField(dhConfig, gFieldID));
    const auto pArray = webrtc::ScopedJavaLocalRef<jbyteArray>::Adopt(env, reinterpret_cast<jbyteArray>(env->GetObjectField(dhConfig, pFieldID)));
    const auto randomArray = webrtc::ScopedJavaLocalRef<jbyteArray>::Adopt(env, reinterpret_cast<jbyteArray>(env->GetObjectField(dhConfig, randomFieldID)));

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
    const auto utfChars = env->GetStringUTFChars(string, nullptr);
    const auto result = std::string(utfChars);
    env->ReleaseStringUTFChars(string, utfChars);
    return result;
}

webrtc::ScopedJavaLocalRef<jstring> parseJString(JNIEnv *env, const std::string &string) {
    return webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, env->NewStringUTF(string.c_str()));
}

bytes::vector parseByteArray(JNIEnv *env, jbyteArray byteArray) {
    if (byteArray == nullptr) {
        return {};
    }
    const jsize length = env->GetArrayLength(byteArray);
    const auto byteBuffer = (uint8_t *) env->GetByteArrayElements(byteArray, nullptr);
    bytes::vector result(length);
    memcpy(&result[0], byteBuffer, length);
    env->ReleaseByteArrayElements(byteArray, (jbyte *) byteBuffer, JNI_ABORT);
    return result;
}

webrtc::ScopedJavaLocalRef<jbyteArray> parseJByteArray(JNIEnv *env, const bytes::vector& byteArray) {
    const auto result = webrtc::ScopedJavaLocalRef<jbyteArray>::Adopt(env, env->NewByteArray(static_cast<jsize>(byteArray.size())));
    env->SetByteArrayRegion(result.obj(), 0,  static_cast<jsize>(byteArray.size()), reinterpret_cast<const jbyte*>(byteArray.data()));
    return result;
}

bytes::binary parseBinary(JNIEnv *env, jbyteArray byteArray) {
    if (byteArray == nullptr) {
        return {};
    }
    const jsize length = env->GetArrayLength(byteArray);
    const auto byteBuffer =  (uint8_t *) env->GetByteArrayElements(byteArray, nullptr);
    bytes::binary result(length);
    memcpy(&result[0], byteBuffer, length);
    env->ReleaseByteArrayElements(byteArray, (jbyte *) byteBuffer, JNI_ABORT);
    return result;
}

webrtc::ScopedJavaLocalRef<jbyteArray> parseJBinary(JNIEnv *env, const bytes::binary& binary) {
    const auto result = webrtc::ScopedJavaLocalRef<jbyteArray>::Adopt(env, env->NewByteArray(static_cast<jsize>(binary.size())));
    env->SetByteArrayRegion(result.obj(), 0, static_cast<jsize>(binary.size()), reinterpret_cast<const jbyte*>(binary.data()));
    return result;
}

bytes::unique_binary parseUniqueBinary(JNIEnv *env, jbyteArray byteArray) {
    if (byteArray == nullptr) {
        return {};
    }
    const jsize length = env->GetArrayLength(byteArray);
    const auto byteBuffer =  (uint8_t *) env->GetByteArrayElements(byteArray, nullptr);
    auto result = bytes::make_unique_binary(length);
    memcpy(result.get(), byteBuffer, length);
    env->ReleaseByteArrayElements(byteArray, (jbyte *) byteBuffer, JNI_ABORT);
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseAuthParams(JNIEnv *env, const ntgcalls::AuthParams& authParams) {
    const auto authParamsClass = webrtc::GetClass(env, "io/github/pytgcalls/p2p/AuthParams");
    jmethodID constructor = env->GetMethodID(authParamsClass.obj(), "<init>", "([BJ)V");
    const auto g_a_or_b = parseJByteArray(env, authParams.g_a_or_b);
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(authParamsClass.obj(), constructor, g_a_or_b.obj(), authParams.key_fingerprint));
}

std::vector<std::string> parseStringList(JNIEnv *env, jobject list) {
    if (list == nullptr) {
        return {};
    }
    const auto listClass = webrtc::GetClass(env, "java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass.obj(), "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass.obj(), "get", "(I)Ljava/lang/Object;");
    std::vector<std::string> result;
    const jint size = env->CallIntMethod(list, sizeMethod);
    for (int i = 0; i < size; i++) {
        const auto element = webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, reinterpret_cast<jstring>(env->CallObjectMethod(list, getMethod, i)));
        result.push_back(parseString(env, element.obj()));
    }
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseJStringList(JNIEnv *env, const std::vector<std::string> &list) {
    const auto arrayListClass = webrtc::GetClass(env, "java/util/ArrayList");
    jmethodID constructor = env->GetMethodID(arrayListClass.obj(), "<init>", "(I)V");
    const auto result = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(arrayListClass.obj(), constructor, static_cast<jint>(list.size())));
    jmethodID addMethod = env->GetMethodID(arrayListClass.obj(), "add", "(Ljava/lang/Object;)Z");
    for (const auto &element : list) {
        const auto string = parseJString(env, element);
        env->CallBooleanMethod(result.obj(), addMethod, string.obj());
    }
    return result;
}

ntgcalls::RTCServer parseRTCServer(JNIEnv *env, jobject rtcServer) {
    const auto dhRTCServerClass = webrtc::GetClass(env, "io/github/pytgcalls/p2p/RTCServer");
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
    const auto ipv4 = webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, reinterpret_cast<jstring>(env->GetObjectField(rtcServer, ipv4FieldID)));
    const auto ipv6 = webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, reinterpret_cast<jstring>(env->GetObjectField(rtcServer, ipv6FieldID)));
    const auto port = static_cast<uint16_t>(env->GetIntField(rtcServer, portFieldID));
    const auto username = webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, reinterpret_cast<jstring>(env->GetObjectField(rtcServer, usernameFieldID)));
    const auto password = webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, reinterpret_cast<jstring>(env->GetObjectField(rtcServer, passwordFieldID)));
    const auto turn = static_cast<bool>(env->GetBooleanField(rtcServer, turnFieldID));
    const auto stun = static_cast<bool>(env->GetBooleanField(rtcServer, stunFieldID));
    const auto tcp = static_cast<bool>(env->GetBooleanField(rtcServer, tcpFieldID));
    const auto peerTag = webrtc::ScopedJavaLocalRef<jbyteArray>::Adopt(env, reinterpret_cast<jbyteArray>(env->GetObjectField(rtcServer, peerTagFieldID)));

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
    const auto listClass = webrtc::GetClass(env, "java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass.obj(), "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass.obj(), "get", "(I)Ljava/lang/Object;");
    std::vector<ntgcalls::RTCServer> result;
    for (int i = 0; i < env->CallIntMethod(list, sizeMethod); i++) {
        const auto element = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->CallObjectMethod(list, getMethod, i));
        result.push_back(parseRTCServer(env, element.obj()));
    }
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseJMediaState(JNIEnv *env, ntgcalls::MediaState mediaState) {
    const auto mediaStateClass = webrtc::GetClass(env, "io/github/pytgcalls/media/MediaState");
    jmethodID constructor = env->GetMethodID(mediaStateClass.obj(), "<init>", "(ZZZ)V");
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(mediaStateClass.obj(), constructor, mediaState.muted, mediaState.videoPaused, mediaState.videoStopped));
}

webrtc::ScopedJavaLocalRef<jobject> parseJProtocol(JNIEnv *env, const ntgcalls::Protocol &protocol) {
    const auto protocolClass = webrtc::GetClass(env, "io/github/pytgcalls/p2p/Protocol");
    jmethodID constructor = env->GetMethodID(protocolClass.obj(), "<init>", "(IIZZLjava/util/List;)V");
    const auto libraryVersions = parseJStringList(env, protocol.library_versions);
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(protocolClass.obj(), constructor, protocol.min_layer, protocol.max_layer, protocol.udp_p2p, protocol.udp_reflector, libraryVersions.obj()));
}

webrtc::ScopedJavaLocalRef<jobject> parseJStreamType(JNIEnv *env, ntgcalls::StreamManager::Type type) {
    const auto streamTypeClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamType");
    jfieldID audioField = env->GetStaticFieldID(streamTypeClass.obj(), "AUDIO", "Lio/github/pytgcalls/media/StreamType;");
    jfieldID videoField = env->GetStaticFieldID(streamTypeClass.obj(), "VIDEO", "Lio/github/pytgcalls/media/StreamType;");

    switch (type) {
        case ntgcalls::StreamManager::Type::Audio:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamTypeClass.obj(), audioField));
        case ntgcalls::StreamManager::Type::Video:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamTypeClass.obj(), videoField));
    }
    return nullptr;
}

webrtc::ScopedJavaLocalRef<jobject> parseJConnectionState(JNIEnv *env, ntgcalls::NetworkInfo::ConnectionState state) {
    const auto connectionStateClass = webrtc::GetClass(env, "io/github/pytgcalls/NetworkInfo$State");
    jfieldID connectingField = env->GetStaticFieldID(connectionStateClass.obj(), "CONNECTING", "Lio/github/pytgcalls/NetworkInfo$State;");
    jfieldID connectedField = env->GetStaticFieldID(connectionStateClass.obj(), "CONNECTED", "Lio/github/pytgcalls/NetworkInfo$State;");
    jfieldID failedField = env->GetStaticFieldID(connectionStateClass.obj(), "FAILED", "Lio/github/pytgcalls/NetworkInfo$State;");
    jfieldID timeoutField = env->GetStaticFieldID(connectionStateClass.obj(), "TIMEOUT", "Lio/github/pytgcalls/NetworkInfo$State;");
    jfieldID closedField = env->GetStaticFieldID(connectionStateClass.obj(), "CLOSED", "Lio/github/pytgcalls/NetworkInfo$State;");

    switch (state) {
        case ntgcalls::NetworkInfo::ConnectionState::Connecting:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(connectionStateClass.obj(), connectingField));
        case ntgcalls::NetworkInfo::ConnectionState::Connected:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env,env->GetStaticObjectField(connectionStateClass.obj(), connectedField));
        case ntgcalls::NetworkInfo::ConnectionState::Failed:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(connectionStateClass.obj(), failedField));
        case ntgcalls::NetworkInfo::ConnectionState::Timeout:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(connectionStateClass.obj(), timeoutField));
        case ntgcalls::NetworkInfo::ConnectionState::Closed:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(connectionStateClass.obj(), closedField));
    }
    return nullptr;
}

webrtc::ScopedJavaLocalRef<jobject> parseJNetworkInfo(JNIEnv *env, ntgcalls::NetworkInfo state) {
    const auto networkInfoClass = webrtc::GetClass(env, "io/github/pytgcalls/NetworkInfo");
    jmethodID constructor = env->GetMethodID(networkInfoClass.obj(), "<init>", "(Lio/github/pytgcalls/NetworkInfo$Kind;Lio/github/pytgcalls/NetworkInfo$State;)V");
    const auto kind = parseJNetworkInfoKind(env, state.kind);
    const auto connectionState = parseJConnectionState(env, state.state);
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(networkInfoClass.obj(), constructor, kind.obj(), connectionState.obj()));
}

webrtc::ScopedJavaLocalRef<jobject> parseJNetworkInfoKind(JNIEnv *env, ntgcalls::NetworkInfo::Kind kind) {
    const auto kindClass = webrtc::GetClass(env, "io/github/pytgcalls/NetworkInfo$Kind");
    jfieldID normalField = env->GetStaticFieldID(kindClass.obj(), "NORMAL", "Lio/github/pytgcalls/NetworkInfo$Kind;");
    jfieldID presentationField = env->GetStaticFieldID(kindClass.obj(), "PRESENTATION", "Lio/github/pytgcalls/NetworkInfo$Kind;");

    switch (kind) {
        case ntgcalls::NetworkInfo::Kind::Normal:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(kindClass.obj(), normalField));
        case ntgcalls::NetworkInfo::Kind::Presentation:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(kindClass.obj(), presentationField));
    }
    return nullptr;
}

webrtc::ScopedJavaLocalRef<jobject> parseJStreamStatus(JNIEnv *env, ntgcalls::StreamManager::Status status) {
    const auto streamStatusClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamStatus");
    jfieldID playingField = env->GetStaticFieldID(streamStatusClass.obj(), "ACTIVE", "Lio/github/pytgcalls/media/StreamStatus;");
    jfieldID pausedField = env->GetStaticFieldID(streamStatusClass.obj(), "PAUSED", "Lio/github/pytgcalls/media/StreamStatus;");
    jfieldID idlingField = env->GetStaticFieldID(streamStatusClass.obj(), "IDLING", "Lio/github/pytgcalls/media/StreamStatus;");

    switch (status) {
        case ntgcalls::StreamManager::Status::Active:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamStatusClass.obj(), playingField));
        case ntgcalls::StreamManager::Status::Paused:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamStatusClass.obj(), pausedField));
        case ntgcalls::StreamManager::Status::Idling:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamStatusClass.obj(), idlingField));
    }
    return nullptr;
}

webrtc::ScopedJavaLocalRef<jobject> parseJCallInfo(JNIEnv *env, const ntgcalls::StreamManager::CallInfo &status) {
    const auto callInfoClass = webrtc::GetClass(env, "io/github/pytgcalls/media/CallInfo");

    jmethodID constructor = env->GetMethodID(callInfoClass.obj(), "<init>", "(Lio/github/pytgcalls/media/StreamStatus;Lio/github/pytgcalls/media/StreamStatus;)V");
    const auto capture = parseJStreamStatus(env, status.capture);
    const auto playback = parseJStreamStatus(env, status.playback);
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(callInfoClass.obj(), constructor, capture.obj(), playback.obj()));
}

webrtc::ScopedJavaLocalRef<jobject> parseJCallInfoMap(JNIEnv *env, const std::map<int64_t, ntgcalls::StreamManager::CallInfo> &calls) {
    const auto mapClass = webrtc::GetClass(env, "java/util/HashMap");
    jmethodID mapConstructor = env->GetMethodID(mapClass.obj(), "<init>", "()V");
    const auto hashMap = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(mapClass.obj(), mapConstructor));
    jmethodID putMethod = env->GetMethodID(mapClass.obj(), "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    const auto longClass = webrtc::GetClass(env, "java/lang/Long");
    jmethodID longConstructor = env->GetMethodID(longClass.obj(), "<init>", "(J)V");
    for (auto const& [key, val] : calls) {
        const auto longKey = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(longClass.obj(), longConstructor, static_cast<jlong>(key)));
        const auto status = parseJCallInfo(env, val);
        env->CallObjectMethod(hashMap.obj(), putMethod, longKey.obj(), status.obj());
    }
    return hashMap;
}

webrtc::ScopedJavaLocalRef<jobject> parseJMediaDevices(JNIEnv *env, const ntgcalls::MediaDevices &devices) {
    const auto mediaDevicesClass = webrtc::GetClass(env, "io/github/pytgcalls/media/MediaDevices");
    jmethodID constructor = env->GetMethodID(mediaDevicesClass.obj(), "<init>", "(Ljava/util/List;Ljava/util/List;Ljava/util/List;Ljava/util/List;)V");
    const auto microphone = parseJDeviceInfoList(env, devices.microphone);
    const auto speaker = parseJDeviceInfoList(env, devices.speaker);
    const auto camera = parseJDeviceInfoList(env, devices.camera);
    const auto screen = parseJDeviceInfoList(env, devices.screen);
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(mediaDevicesClass.obj(), constructor, microphone.obj(), speaker.obj(), camera.obj(), screen.obj()));
}

webrtc::ScopedJavaLocalRef<jobject> parseJDeviceInfoList(JNIEnv *env, const std::vector<ntgcalls::DeviceInfo> &devices) {
    const auto arrayListClass = webrtc::GetClass(env, "java/util/ArrayList");
    jmethodID constructor = env->GetMethodID(arrayListClass.obj(), "<init>", "(I)V");
    const auto result = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(arrayListClass.obj(), constructor, static_cast<jint>(devices.size())));
    jmethodID addMethod = env->GetMethodID(arrayListClass.obj(), "add", "(Ljava/lang/Object;)Z");
    for (const auto &element : devices) {
        const auto device = parseJDeviceInfo(env, element);
        env->CallBooleanMethod(result.obj(), addMethod, device.obj());
    }
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseJDeviceInfo(JNIEnv *env, const ntgcalls::DeviceInfo& device) {
    const auto deviceInfoClass = webrtc::GetClass(env, "io/github/pytgcalls/media/DeviceInfo");
    jmethodID constructor = env->GetMethodID(deviceInfoClass.obj(), "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
    const auto deviceName = parseJString(env, device.name);
    const auto metadata = parseJString(env, device.metadata);
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(deviceInfoClass.obj(), constructor, deviceName.obj(), metadata.obj()));
}

ntgcalls::StreamManager::Mode parseStreamMode(JNIEnv *env, jobject device) {
    const auto streamModeClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamMode");
    jfieldID captureField = env->GetStaticFieldID(streamModeClass.obj(), "CAPTURE", "Lio/github/pytgcalls/media/StreamMode;");
    jfieldID playbackField = env->GetStaticFieldID(streamModeClass.obj(), "PLAYBACK", "Lio/github/pytgcalls/media/StreamMode;");
    const auto playback = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), playbackField));
    const auto capture = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), captureField));

    if (env->IsSameObject(device, capture.obj())) {
        return ntgcalls::StreamManager::Mode::Capture;
    } else if (env->IsSameObject(device, playback.obj())) {
        return ntgcalls::StreamManager::Mode::Playback;
    }
    return ntgcalls::StreamManager::Mode::Capture;
}

webrtc::ScopedJavaLocalRef<jobject> parseJStreamMode(JNIEnv *env, ntgcalls::StreamManager::Mode mode) {
    const auto streamModeClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamMode");
    jfieldID captureField = env->GetStaticFieldID(streamModeClass.obj(), "CAPTURE", "Lio/github/pytgcalls/media/StreamMode;");
    jfieldID playbackField = env->GetStaticFieldID(streamModeClass.obj(), "PLAYBACK", "Lio/github/pytgcalls/media/StreamMode;");
    switch (mode) {
        case ntgcalls::StreamManager::Mode::Capture:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), captureField));
        case ntgcalls::StreamManager::Mode::Playback:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), playbackField));
    }
    return nullptr;
}

ntgcalls::StreamManager::Device parseDevice(JNIEnv *env, jobject device) {
    const auto streamDeviceClass = webrtc::GetClass(env,"io/github/pytgcalls/media/StreamDevice");
    jfieldID microphoneField = env->GetStaticFieldID(streamDeviceClass.obj(), "MICROPHONE", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID speakerField = env->GetStaticFieldID(streamDeviceClass.obj(), "SPEAKER", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID cameraField = env->GetStaticFieldID(streamDeviceClass.obj(), "CAMERA", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID screenField = env->GetStaticFieldID(streamDeviceClass.obj(), "SCREEN", "Lio/github/pytgcalls/media/StreamDevice;");
    const auto microphone = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamDeviceClass.obj(), microphoneField));
    const auto speaker = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamDeviceClass.obj(), speakerField));
    const auto camera = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamDeviceClass.obj(), cameraField));
    const auto screen = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamDeviceClass.obj(), screenField));

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
    const auto streamDeviceClass = webrtc::GetClass(env,"io/github/pytgcalls/media/StreamDevice");
    jfieldID microphoneField = env->GetStaticFieldID(streamDeviceClass.obj(), "MICROPHONE", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID speakerField = env->GetStaticFieldID(streamDeviceClass.obj(), "SPEAKER", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID cameraField = env->GetStaticFieldID(streamDeviceClass.obj(), "CAMERA", "Lio/github/pytgcalls/media/StreamDevice;");
    jfieldID screenField = env->GetStaticFieldID(streamDeviceClass.obj(), "SCREEN", "Lio/github/pytgcalls/media/StreamDevice;");

    switch (device) {
        case ntgcalls::StreamManager::Device::Microphone:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamDeviceClass.obj(), microphoneField));
        case ntgcalls::StreamManager::Device::Speaker:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamDeviceClass.obj(), speakerField));
        case ntgcalls::StreamManager::Device::Camera:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamDeviceClass.obj(), cameraField));
        case ntgcalls::StreamManager::Device::Screen:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamDeviceClass.obj(), screenField));
    }
    return nullptr;
}

wrtc::FrameData parseFrameData(JNIEnv *env, jobject frameData) {
    const auto frameDataClass = webrtc::GetClass(env, "io/github/pytgcalls/media/FrameData");
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
    const auto frameDataClass = webrtc::GetClass(env,"io/github/pytgcalls/media/FrameData");
    jmethodID constructor = env->GetMethodID(frameDataClass.obj(), "<init>", "(JIII)V");
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(frameDataClass.obj(), constructor, frameData.absoluteCaptureTimestampMs, static_cast<jint>(frameData.width), static_cast<jint>(frameData.height), static_cast<jint>(frameData.rotation)));
}

webrtc::ScopedJavaLocalRef<jobject> parseJRemoteSource(JNIEnv *env, const ntgcalls::RemoteSource& source) {
    const auto remoteSourceClass = webrtc::GetClass(env,"io/github/pytgcalls/media/RemoteSource");
    jmethodID constructor = env->GetMethodID(remoteSourceClass.obj(), "<init>", "(ILio/github/pytgcalls/media/StreamStatus;Lio/github/pytgcalls/media/StreamDevice;)V");
    const auto sourceStateClass = webrtc::GetClass(env, "io/github/pytgcalls/media/StreamStatus");
    webrtc::ScopedJavaLocalRef<jobject> state = parseJStreamStatus(env, source.state);
    const auto device = parseJDevice(env, source.device);
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(remoteSourceClass.obj(), constructor, source.ssrc, state.obj(), device.obj()));
}

wrtc::SsrcGroup parseSsrcGroup(JNIEnv *env, jobject ssrcGroup) {
    const auto ssrcGroupClass = webrtc::GetClass(env, "io/github/pytgcalls/media/SsrcGroup");
    jfieldID semanticsField = env->GetFieldID(ssrcGroupClass.obj(), "semantics", "Ljava/lang/String;");
    jfieldID ssrcGroupsField = env->GetFieldID(ssrcGroupClass.obj(), "ssrcGroups", "Ljava/util/List;");

    const auto semantics = webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, reinterpret_cast<jstring>(env->GetObjectField(ssrcGroup, semanticsField)));
    const auto ssrcGroups = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetObjectField(ssrcGroup, ssrcGroupsField));
    const auto listClass = webrtc::GetClass(env, "java/util/List");

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
    const auto listClass = webrtc::GetClass(env, "java/util/List");
    jmethodID sizeMethod = env->GetMethodID(listClass.obj(), "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass.obj(), "get", "(I)Ljava/lang/Object;");
    std::vector<wrtc::SsrcGroup> result;
    for (int i = 0; i < env->CallIntMethod(list, sizeMethod); i++) {
        const auto element = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->CallObjectMethod(list, getMethod, i));
        result.push_back(parseSsrcGroup(env, element.obj()));
    }
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseJFrame(JNIEnv *env, const wrtc::Frame& frame) {
    const auto frameClass = webrtc::GetClass(env,"io/github/pytgcalls/media/Frame");
    jmethodID constructor = env->GetMethodID(frameClass.obj(), "<init>", "(J[BLio/github/pytgcalls/media/FrameData;)V");
    const auto data = parseJBinary(env, frame.data);
    const auto frameData = parseJFrameData(env, frame.frameData);
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(frameClass.obj(), constructor, frame.ssrc, data.obj(), frameData.obj()));
}

webrtc::ScopedJavaLocalRef<jobject> parseJFrames(JNIEnv *env, const std::vector<wrtc::Frame>& frame) {
    const auto arrayListClass = webrtc::GetClass(env, "java/util/ArrayList");
    jmethodID constructor = env->GetMethodID(arrayListClass.obj(), "<init>", "(I)V");
    const auto result = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(arrayListClass.obj(), constructor, static_cast<jint>(frame.size())));
    jmethodID addMethod = env->GetMethodID(arrayListClass.obj(), "add", "(Ljava/lang/Object;)Z");
    for (const auto &element : frame) {
        const auto jFrame = parseJFrame(env, element);
        env->CallBooleanMethod(result.obj(), addMethod, jFrame.obj());
    }
    return result;
}

webrtc::ScopedJavaLocalRef<jobject> parseJSegmentPartRequest(JNIEnv *env, const wrtc::SegmentPartRequest& request) {
    const auto segmentPartRequestCalls = webrtc::GetClass(env,"io/github/pytgcalls/media/SegmentPartRequest");
    jmethodID constructor = env->GetMethodID(segmentPartRequestCalls.obj(), "<init>", "(JIIJZILio/github/pytgcalls/media/MediaSegmentQuality;)V");
    const auto quality = parseJMediaSegmentQuality(env, request.quality);
    return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->NewObject(segmentPartRequestCalls.obj(), constructor, request.segmentId, request.partId, request.limit, request.timestamp, request.qualityUpdate, request.channelId, quality.obj()));
}

webrtc::ScopedJavaLocalRef<jobject> parseJMediaSegmentQuality(JNIEnv *env, const wrtc::MediaSegment::Quality& quality) {
    const auto streamStatusClass = webrtc::GetClass(env, "io/github/pytgcalls/media/MediaSegmentQuality");
    jfieldID noneField = env->GetStaticFieldID(streamStatusClass.obj(), "NONE", "Lio/github/pytgcalls/media/MediaSegmentQuality;");
    jfieldID thumbnailField = env->GetStaticFieldID(streamStatusClass.obj(), "THUMBNAIL", "Lio/github/pytgcalls/media/MediaSegmentQuality;");
    jfieldID mediumField = env->GetStaticFieldID(streamStatusClass.obj(), "MEDIUM", "Lio/github/pytgcalls/media/MediaSegmentQuality;");
    jfieldID fullField = env->GetStaticFieldID(streamStatusClass.obj(), "FULL", "Lio/github/pytgcalls/media/MediaSegmentQuality;");

    switch (quality) {
        case wrtc::MediaSegment::Quality::None:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamStatusClass.obj(), noneField));
        case wrtc::MediaSegment::Quality::Thumbnail:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamStatusClass.obj(), thumbnailField));
        case wrtc::MediaSegment::Quality::Medium:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamStatusClass.obj(), mediumField));
        case wrtc::MediaSegment::Quality::Full:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamStatusClass.obj(), fullField));
    }
    return nullptr;
}

wrtc::MediaSegment::Part::Status parseSegmentPartStatus(JNIEnv *env, jobject status) {
    const auto streamModeClass = webrtc::GetClass(env, "io/github/pytgcalls/media/MediaSegmentStatus");
    jfieldID fileField = env->GetStaticFieldID(streamModeClass.obj(), "NOT_READY", "Lio/github/pytgcalls/media/MediaSegmentStatus;");
    jfieldID resyncNeededField = env->GetStaticFieldID(streamModeClass.obj(), "RESYNC_NEEDED", "Lio/github/pytgcalls/media/MediaSegmentStatus;");
    jfieldID successField = env->GetStaticFieldID(streamModeClass.obj(), "SUCCESS", "Lio/github/pytgcalls/media/MediaSegmentStatus;");
    const auto notReady = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), fileField));
    const auto resyncNeeded = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), resyncNeededField));
    const auto success = webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(streamModeClass.obj(), successField));

    if (env->IsSameObject(status, notReady.obj())) {
        return wrtc::MediaSegment::Part::Status::NotReady;
    } else if (env->IsSameObject(status, resyncNeeded.obj())) {
        return wrtc::MediaSegment::Part::Status::ResyncNeeded;
    } else if (env->IsSameObject(status, success.obj())) {
        return wrtc::MediaSegment::Part::Status::Success;
    }
    return wrtc::MediaSegment::Part::Status::NotReady;
}

webrtc::ScopedJavaLocalRef<jobject> parseJConnectionMode(JNIEnv *env, wrtc::ConnectionMode mode) {
    const auto connectionModeClass = webrtc::GetClass(env, "io/github/pytgcalls/ConnectionMode");
    jfieldID rtcField = env->GetStaticFieldID(connectionModeClass.obj(), "RTC", "Lio/github/pytgcalls/ConnectionMode;");
    jfieldID streamField = env->GetStaticFieldID(connectionModeClass.obj(), "STREAM", "Lio/github/pytgcalls/ConnectionMode;");
    jfieldID rtmpField = env->GetStaticFieldID(connectionModeClass.obj(), "RTMP", "Lio/github/pytgcalls/ConnectionMode;");

    switch (mode) {
        case wrtc::ConnectionMode::Rtc:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(connectionModeClass.obj(), rtcField));
        case wrtc::ConnectionMode::Stream:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(connectionModeClass.obj(), streamField));
        case wrtc::ConnectionMode::Rtmp:
            return webrtc::ScopedJavaLocalRef<>::Adopt(env, env->GetStaticObjectField(connectionModeClass.obj(), rtmpField));
        case wrtc::ConnectionMode::None:
            break;
    }
    return nullptr;
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
    const auto exceptionClass = webrtc::GetClass(env, name.c_str());
    if (!exceptionClass.is_null()) {
        env->ThrowNew(exceptionClass.obj(), message.c_str());
    }
}
