#include "utils.hpp"
#include <string>

extern "C"
JNIEXPORT jstring JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_pingNative(JNIEnv* env, jclass) {
    return env->NewStringUTF(ntgcalls::NTgCalls::ping().c_str());
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_init(JNIEnv *env, jobject obj) {
    jclass clazz = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(clazz, "nativePointer", "J");
    env->SetLongField(obj, fid, reinterpret_cast<jlong>(new ntgcalls::NTgCalls()));
}

extern "C"
JNIEXPORT void JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_destroy(JNIEnv *env, jobject obj) {
    jclass clazz = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(clazz, "nativePointer", "J");
    jlong ptr = env->GetLongField(obj, fid);
    if (ptr != 0) {
        auto* cppObject = reinterpret_cast<ntgcalls::NTgCalls*>(ptr);
        delete cppObject;
        env->SetLongField(obj, fid, static_cast<jlong>(static_cast<long>(0)));
    }
}

extern "C"
JNIEXPORT jstring JNICALL Java_org_pytgcalls_ntgcalls_NTgCalls_createCall(JNIEnv *env, jobject obj, jlong chatId, jobject mediaDescription) {
    try {
        auto instance = getInstance(env, obj);
        return env->NewStringUTF(instance->createCall(static_cast<long>(chatId), parseMediaDescription(env, mediaDescription)).c_str());
    } catch (const std::exception& e) {
        throwJavaException(env, "RuntimeException", e.what());
    }
    return nullptr;
}
