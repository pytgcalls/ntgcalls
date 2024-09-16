#include "utils.hpp"
#include <string>

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_init(JNIEnv *env, jobject thiz) {
    jclass clazz = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(clazz, "nativePointer", "J");
    env->SetLongField(thiz, fid, reinterpret_cast<jlong>(new ntgcalls::NTgCalls()));
    env->DeleteLocalRef(clazz);
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
    env->DeleteLocalRef(clazz);
}

extern "C"
JNIEXPORT jbyteArray JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_createP2PCall(JNIEnv *env, jobject thiz, jlong chatId, jobject dhConfig, jbyteArray g_a_hash, jobject media_description) {
    try {
        auto instance = getInstance(env, thiz);
        return parseJByteArray(env, instance ->createP2PCall(static_cast<long>(chatId), parseDhConfig(env, dhConfig), parseByteArray(env, g_a_hash), parseMediaDescription(env, media_description)));
    } HANDLE_EXCEPTIONS
    return nullptr;
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
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_changeStream(JNIEnv *env, jobject thiz, jlong chatId, jobject mediaDescription) {
    try {
        auto instance = getInstance(env, thiz);
        instance->changeStream(static_cast<long>(chatId), parseMediaDescription(env, mediaDescription));
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
JNIEXPORT jlong JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_time(JNIEnv *env, jobject thiz, jlong chatId) {
    try {
        auto instance = getInstance(env, thiz);
        return static_cast<jlong>(instance->time(static_cast<long>(chatId)));
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
