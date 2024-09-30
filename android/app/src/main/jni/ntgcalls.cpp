#include "utils.hpp"
#include <string>
#include <wrtc/utils/java_context.hpp>

std::map<jlong, InstanceCallbacks> callbacksInstances;

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_init(JNIEnv *env, jobject thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(clazz, "nativePointer", "J");
    env->SetLongField(thiz, fid, reinterpret_cast<jlong>(new ntgcalls::NTgCalls()));
    env->DeleteLocalRef(clazz);

    auto instancePtr = getInstancePtr(env, thiz);
    auto instance = reinterpret_cast<ntgcalls::NTgCalls*>(instancePtr);
    callbacksInstances[instancePtr] = InstanceCallbacks();

    instance->onUpgrade([instancePtr](int64_t chatId, ntgcalls::MediaState mediaState) {
        auto callback = callbacksInstances[instancePtr].onUpgradeCallback;
        if (!callback) {
            return;
        }
        auto env = (JNIEnv*) wrtc::GetJNIEnv();
        env->CallVoidMethod(callback->callback, callback->methodId, static_cast<jlong>(chatId), parseMediaState(env, mediaState));
    });

    instance->onStreamEnd([instancePtr](int64_t chatId, ntgcalls::StreamManager::Type type, ntgcalls::StreamManager::Device device) {
        auto callback = callbacksInstances[instancePtr].onStreamEndCallback;
        if (!callback) {
            return;
        }
        auto env = (JNIEnv*) wrtc::GetJNIEnv();
        env->CallVoidMethod(callback->callback, callback->methodId, static_cast<jlong>(chatId), parseStreamType(env, type), parseJDevice(env, device));
    });

    instance->onConnectionChange([instancePtr](int64_t chatId, ntgcalls::CallInterface::ConnectionState state) {
        auto callback = callbacksInstances[instancePtr].onConnectionChangeCallback;
        if (!callback) {
            return;
        }
        auto env = (JNIEnv*) wrtc::GetJNIEnv();
        env->CallVoidMethod(callback->callback, callback->methodId, static_cast<jlong>(chatId), parseConnectionState(env, state));
    });

    instance->onSignalingData([instancePtr](int64_t chatId, const bytes::binary& data) {
        auto callback = callbacksInstances[instancePtr].onSignalingDataCallback;
        if (!callback) {
            return;
        }
        auto env = (JNIEnv*) wrtc::GetJNIEnv();
        env->CallVoidMethod(callback->callback, callback->methodId, static_cast<jlong>(chatId), parseJBinary(env, data));
    });
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_destroy(JNIEnv *env, jobject thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(clazz, "nativePointer", "J");
    jlong ptr = env->GetLongField(thiz, fid);
    if (ptr != 0) {
        auto* cppObject = reinterpret_cast<ntgcalls::NTgCalls*>(ptr);
        delete cppObject;
        env->SetLongField(thiz, fid, static_cast<jlong>(static_cast<long>(0)));
    }
    auto callbackInfo = callbacksInstances[ptr];
    if (auto callback = callbackInfo.onUpgradeCallback) {
        env->DeleteGlobalRef(callback->callback);
    }
    if (auto callback = callbackInfo.onStreamEndCallback) {
        env->DeleteGlobalRef(callback->callback);
    }
    if (auto callback = callbackInfo.onConnectionChangeCallback) {
        env->DeleteGlobalRef(callback->callback);
    }
    if (auto callback = callbackInfo.onSignalingDataCallback) {
        env->DeleteGlobalRef(callback->callback);
    }
    callbacksInstances.erase(ptr);
    env->DeleteLocalRef(clazz);
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_createP2PCall(JNIEnv *env, jobject thiz, jlong chatId, jobject media_description) {
    try {
        auto instance = getInstance(env, thiz);
        instance ->createP2PCall(static_cast<long>(chatId), parseMediaDescription(env, media_description));
    } HANDLE_EXCEPTIONS
}

extern "C"
JNIEXPORT jbyteArray JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_initExchange(JNIEnv *env, jobject thiz, jlong chatId, jobject dhConfig, jbyteArray g_a_hash) {
    try {
        auto instance = getInstance(env, thiz);
        return parseJByteArray(env, instance ->initExchange(static_cast<long>(chatId), parseDhConfig(env, dhConfig), parseByteArray(env, g_a_hash)));
    } HANDLE_EXCEPTIONS
    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_skipExchange(JNIEnv *env, jobject thiz, jlong chatId, jbyteArray encryptionKey, jboolean isOutgoing) {
    try {
        auto instance = getInstance(env, thiz);
        instance ->skipExchange(static_cast<long>(chatId), parseByteArray(env, encryptionKey), isOutgoing);
    } HANDLE_EXCEPTIONS
}

extern "C"
JNIEXPORT jobject JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_exchangeKeys(JNIEnv *env, jobject thiz, jlong chat_id, jbyteArray g_a_or_b, jint key_fingerprint) {
    try {
        auto instance = getInstance(env, thiz);
        return parseAuthParams(env, instance->exchangeKeys(static_cast<long>(chat_id), parseByteArray(env, g_a_or_b), static_cast<int64_t>(key_fingerprint)));
    } HANDLE_EXCEPTIONS
    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_connectP2P(JNIEnv *env, jobject thiz, jlong chat_id, jobject rtc_servers, jobject versions, jboolean p2p_allowed) {
    try {
        auto instance = getInstance(env, thiz);
        instance->connectP2P(static_cast<long>(chat_id), parseRTCServerList(env, rtc_servers), parseStringList(env, versions), static_cast<bool>(p2p_allowed));
    } HANDLE_EXCEPTIONS
}

extern "C"
JNIEXPORT jstring JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_createCall(JNIEnv *env, jobject thiz, jlong chatId, jobject mediaDescription) {
    try {
        auto instance = getInstance(env, thiz);
        return parseJString(env, instance->createCall(static_cast<long>(chatId),parseMediaDescription(env, mediaDescription)));
    } HANDLE_EXCEPTIONS
    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_connect(JNIEnv *env, jobject thiz, jlong chatId, jstring params) {
    try {
        auto instance = getInstance(env, thiz);
        instance->connect(static_cast<long>(chatId), parseString(env, params));
    } HANDLE_EXCEPTIONS
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_setStreamSources(JNIEnv *env, jobject thiz, jlong chatId, jobject mode, jobject mediaDescription) {
    try {
        auto instance = getInstance(env, thiz);
        instance->setStreamSources(static_cast<long>(chatId), parseStreamMode(env, mode), parseMediaDescription(env, mediaDescription));
    } HANDLE_EXCEPTIONS
}

extern "C"
JNIEXPORT jboolean JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_pause(JNIEnv *env, jobject thiz, jlong chatId) {
    try {
        auto instance = getInstance(env, thiz);
        return static_cast<jboolean>(instance->pause(static_cast<long>(chatId)));
    } HANDLE_EXCEPTIONS
    return static_cast<jboolean>(false);
}

extern "C"
JNIEXPORT jboolean JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_resume(JNIEnv *env, jobject thiz, jlong chatId) {
    try {
        auto instance = getInstance(env, thiz);
        return static_cast<jboolean>(instance->resume(static_cast<long>(chatId)));
    } HANDLE_EXCEPTIONS
    return static_cast<jboolean>(false);
}

extern "C"
JNIEXPORT jboolean JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_mute(JNIEnv *env, jobject thiz, jlong chatId) {
    try {
        auto instance = getInstance(env, thiz);
        return static_cast<jboolean>(instance->mute(static_cast<long>(chatId)));
    } HANDLE_EXCEPTIONS
    return static_cast<jboolean>(false);
}

extern "C"
JNIEXPORT jboolean JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_unmute(JNIEnv *env, jobject thiz, jlong chatId) {
    try {
        auto instance = getInstance(env, thiz);
        return static_cast<jboolean>(instance->unmute(static_cast<long>(chatId)));
    } HANDLE_EXCEPTIONS
    return static_cast<jboolean>(false);
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_stop(JNIEnv *env, jobject thiz, jlong chatId) {
    try {
        auto instance = getInstance(env, thiz);
        instance->stop(static_cast<long>(chatId));
    } HANDLE_EXCEPTIONS
}

extern "C"
JNIEXPORT jlong JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_time(JNIEnv *env, jobject thiz, jlong chatId, jobject mode) {
    try {
        auto instance = getInstance(env, thiz);
        return static_cast<jlong>(instance->time(static_cast<long>(chatId), parseStreamMode(env, mode)));
    } HANDLE_EXCEPTIONS
    return static_cast<jlong>(0);
}

extern "C"
JNIEXPORT jobject JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_getState(JNIEnv *env, jobject thiz, jlong chat_id) {
    try {
        auto instance = getInstance(env, thiz);
        return parseMediaState(env, instance->getState(static_cast<long>(chat_id)));
    } HANDLE_EXCEPTIONS
    return nullptr;
}

extern "C"
JNIEXPORT jstring JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_pingNative(JNIEnv* env, jclass) {
    return env->NewStringUTF(ntgcalls::NTgCalls::ping().c_str());
}

extern "C"
JNIEXPORT jobject JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_getProtocol(JNIEnv* env, jclass) {
    return parseProtocol(env, ntgcalls::NTgCalls::getProtocol());
}

REGISTER_CALLBACK(setUpgradeCallback, onUpgrade, "(JLorg/pytgcalls/ntgcalls/media/MediaState;)V")

REGISTER_CALLBACK(setStreamEndCallback, onStreamEnd, "(JLorg/pytgcalls/ntgcalls/media/StreamType;)V")

REGISTER_CALLBACK(setConnectionChangeCallback, onConnectionChange, "(JLorg/pytgcalls/ntgcalls/ConnectionState;)V")

REGISTER_CALLBACK(setSignalingDataCallback, onSignalingData, "(J[B)V")

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_sendSignalingData(JNIEnv *env, jobject thiz, jlong chat_id, jbyteArray data) {
    try {
        auto instance = getInstance(env, thiz);
        instance->sendSignalingData(static_cast<long>(chat_id), parseBinary(env, data));
    } HANDLE_EXCEPTIONS
}

extern "C"
JNIEXPORT jobject JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_getMediaDevices(JNIEnv *env, jclass) {
    return parseMediaDevices(env, ntgcalls::NTgCalls::getMediaDevices());
}

extern "C"
JNIEXPORT jobject JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_calls(JNIEnv *env, jobject thiz) {
    auto instance = getInstance(env, thiz);
    return parseMediaStatusMap(env, instance->calls());
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_devices_JavaAudioDeviceModule_onRecordedData(JNIEnv *env, jobject thiz, jbyteArray data) {
    auto instance = getInstanceAudioCapture(env, thiz);
    instance->onRecordedData(std::move(parseUniqueBinary(env, data)));
}