@config java_pkg = io.github.pytgcalls
@typemap Vector<*> = jobject
@typemap Map<*, *> = jobject
@wrapmap jnidesc long = J
@wrapmap jnidesc ulong = J
@wrapmap jnidesc int = I
@wrapmap jnidesc uint = I
@wrapmap jnidesc int8 = I
@wrapmap jnidesc uint8 = I
@wrapmap jnidesc int16 = I
@wrapmap jnidesc uint16 = I
@wrapmap jnidesc bool = Z
@wrapmap jnidesc double = D
@wrapmap jnidesc bytes = [B
@wrapmap jnidesc string = Ljava/lang/String;
@wrapmap jnidesc Vector<*> = Ljava/util/List;
@wrapmap jnidesc Map<*, *> = Ljava/util/Map;
@wrapmap jnidesc * = Lio/github/pytgcalls/@;
@wrapmap jget long = GetLongField
@wrapmap jget ulong = GetLongField
@wrapmap jget int = GetIntField
@wrapmap jget uint = GetIntField
@wrapmap jget int8 = GetIntField
@wrapmap jget uint8 = GetIntField
@wrapmap jget int16 = GetIntField
@wrapmap jget uint16 = GetIntField
@wrapmap jget bool = GetBooleanField
@wrapmap jget double = GetDoubleField
@wrapmap jcast long = jlong
@wrapmap jcast ulong = jlong
@wrapmap jcast int = jint
@wrapmap jcast uint = jint
@wrapmap jcast int8 = jint
@wrapmap jcast uint8 = jint
@wrapmap jcast int16 = jint
@wrapmap jcast uint16 = jint
@wrapmap jcast bool = jboolean
@wrapmap jcast double = jdouble
#pragma once
#include <jni.h>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <ntgcalls/ntgcalls.hpp>
#include <ntgcalls/exceptions.hpp>
#include <sdk/android/native_api/jni/scoped_java_ref.h>
#include <wrtc/utils/java_context.hpp>

#pragma clang diagnostic ignored "-Wunused-function"

#define CAPTURE_JAVA_EXCEPTION if (env->ExceptionCheck()) { \
    env->ExceptionDescribe(); \
    env->ExceptionClear(); \
}

struct JavaCallback {
    jobject callback;
    jmethodID methodId;
};

inline webrtc::ScopedJavaLocalRef<jclass> findClass(JNIEnv* env, const char* name) {
    return webrtc::ScopedJavaLocalRef<jclass>::Adopt(env, env->FindClass(name));
}

inline void throwJavaException(JNIEnv* env, const std::string& name, const std::string& message) {
    if (const auto clazz = findClass(env, name.c_str()); clazz.obj() != nullptr) {
        env->ThrowNew(clazz.obj(), message.c_str());
    }
}

inline jlong getInstancePtr(JNIEnv* env, jobject obj) {
    const auto clazz = webrtc::ScopedJavaLocalRef<jclass>::Adopt(env, env->GetObjectClass(obj));
    return env->GetLongField(obj, env->GetFieldID(clazz.obj(), "nativePointer", "J"));
}

inline ntgcalls::NTgCalls* getInstance(JNIEnv* env, jobject obj) {
    const auto ptr = getInstancePtr(env, obj);
    return ptr != 0 ? reinterpret_cast<ntgcalls::NTgCalls*>(ptr) : nullptr;
}

inline std::string parseString(JNIEnv* env, jstring value) {
    if (value == nullptr) {
        return {};
    }
    const char* chars = env->GetStringUTFChars(value, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

inline webrtc::ScopedJavaLocalRef<jstring> parseJString(JNIEnv* env, const std::string& value) {
    return webrtc::ScopedJavaLocalRef<jstring>::Adopt(env, env->NewStringUTF(value.c_str()));
}

inline bytes::binary parseByteArray(JNIEnv* env, jbyteArray value) {
    if (value == nullptr) {
        return {};
    }
    const jsize length = env->GetArrayLength(value);
    bytes::binary result(length);
    env->GetByteArrayRegion(value, 0, length, reinterpret_cast<jbyte*>(result.data()));
    return result;
}

inline webrtc::ScopedJavaLocalRef<jbyteArray> parseJByteArray(JNIEnv* env, const bytes::binary& value) {
    auto array = webrtc::ScopedJavaLocalRef<jbyteArray>::Adopt(env, env->NewByteArray(static_cast<jsize>(value.size())));
    env->SetByteArrayRegion(array.obj(), 0, static_cast<jsize>(value.size()), reinterpret_cast<const jbyte*>(value.data()));
    return array;
}

@for e in enums
@if e.emit
inline @{e.cpp} parse@{e.name}(JNIEnv* env, jobject value);
inline webrtc::ScopedJavaLocalRef<jobject> parseJ@{e.name}(JNIEnv* env, @{e.cpp} value);
@end
@end
@for s in structs
inline @{s.cpp} parse@{s.name}(JNIEnv* env, jobject value);
inline webrtc::ScopedJavaLocalRef<jobject> parseJ@{s.name}(JNIEnv* env, const @{s.cpp}& value);
@end

inline std::string convElem(JNIEnv* env, jobject value, std::string*) { return parseString(env, static_cast<jstring>(value)); }
inline bytes::binary convElem(JNIEnv* env, jobject value, bytes::binary*) { return parseByteArray(env, static_cast<jbyteArray>(value)); }
inline jobject convElemJ(JNIEnv* env, const std::string& value) { return parseJString(env, value).Release(); }
inline jobject convElemJ(JNIEnv* env, const bytes::binary& value) { return parseJByteArray(env, value).Release(); }
inline int32_t convElem(JNIEnv* env, jobject value, int32_t*) { const auto c = findClass(env, "java/lang/Integer"); return env->CallIntMethod(value, env->GetMethodID(c.obj(), "intValue", "()I")); }
inline uint32_t convElem(JNIEnv* env, jobject value, uint32_t*) { const auto c = findClass(env, "java/lang/Integer"); return static_cast<uint32_t>(env->CallIntMethod(value, env->GetMethodID(c.obj(), "intValue", "()I"))); }
inline int64_t convElem(JNIEnv* env, jobject value, int64_t*) { const auto c = findClass(env, "java/lang/Long"); return env->CallLongMethod(value, env->GetMethodID(c.obj(), "longValue", "()J")); }
inline uint64_t convElem(JNIEnv* env, jobject value, uint64_t*) { const auto c = findClass(env, "java/lang/Long"); return static_cast<uint64_t>(env->CallLongMethod(value, env->GetMethodID(c.obj(), "longValue", "()J"))); }
inline double convElem(JNIEnv* env, jobject value, double*) { const auto c = findClass(env, "java/lang/Double"); return env->CallDoubleMethod(value, env->GetMethodID(c.obj(), "doubleValue", "()D")); }
inline bool convElem(JNIEnv* env, jobject value, bool*) { const auto c = findClass(env, "java/lang/Boolean"); return env->CallBooleanMethod(value, env->GetMethodID(c.obj(), "booleanValue", "()Z")); }
inline jobject convElemJ(JNIEnv* env, int32_t value) { const auto c = findClass(env, "java/lang/Integer"); return env->NewObject(c.obj(), env->GetMethodID(c.obj(), "<init>", "(I)V"), static_cast<jint>(value)); }
inline jobject convElemJ(JNIEnv* env, uint32_t value) { const auto c = findClass(env, "java/lang/Integer"); return env->NewObject(c.obj(), env->GetMethodID(c.obj(), "<init>", "(I)V"), static_cast<jint>(value)); }
inline jobject convElemJ(JNIEnv* env, int64_t value) { const auto c = findClass(env, "java/lang/Long"); return env->NewObject(c.obj(), env->GetMethodID(c.obj(), "<init>", "(J)V"), static_cast<jlong>(value)); }
inline jobject convElemJ(JNIEnv* env, uint64_t value) { const auto c = findClass(env, "java/lang/Long"); return env->NewObject(c.obj(), env->GetMethodID(c.obj(), "<init>", "(J)V"), static_cast<jlong>(value)); }
inline jobject convElemJ(JNIEnv* env, double value) { const auto c = findClass(env, "java/lang/Double"); return env->NewObject(c.obj(), env->GetMethodID(c.obj(), "<init>", "(D)V"), static_cast<jdouble>(value)); }
inline jobject convElemJ(JNIEnv* env, bool value) { const auto c = findClass(env, "java/lang/Boolean"); return env->NewObject(c.obj(), env->GetMethodID(c.obj(), "<init>", "(Z)V"), static_cast<jboolean>(value)); }
@for s in structs
inline @{s.cpp} convElem(JNIEnv* env, jobject value, @{s.cpp}*) { return parse@{s.name}(env, value); }
inline jobject convElemJ(JNIEnv* env, const @{s.cpp}& value) { return parseJ@{s.name}(env, value).Release(); }
@end

template <typename T>
std::vector<T> parseList(JNIEnv* env, jobject list) {
    std::vector<T> result;
    if (list == nullptr) {
        return result;
    }
    const auto listClass = findClass(env, "java/util/List");
    const jmethodID sizeMethod = env->GetMethodID(listClass.obj(), "size", "()I");
    const jmethodID getMethod = env->GetMethodID(listClass.obj(), "get", "(I)Ljava/lang/Object;");
    const jint size = env->CallIntMethod(list, sizeMethod);
    result.reserve(size);
    for (jint i = 0; i < size; ++i) {
        jobject element = env->CallObjectMethod(list, getMethod, i);
        result.push_back(convElem(env, element, static_cast<T*>(nullptr)));
        env->DeleteLocalRef(element);
    }
    return result;
}

template <typename T>
webrtc::ScopedJavaLocalRef<jobject> parseJList(JNIEnv* env, const std::vector<T>& values) {
    const auto arrayListClass = findClass(env, "java/util/ArrayList");
    const jmethodID ctor = env->GetMethodID(arrayListClass.obj(), "<init>", "()V");
    const jmethodID addMethod = env->GetMethodID(arrayListClass.obj(), "add", "(Ljava/lang/Object;)Z");
    auto list = webrtc::ScopedJavaLocalRef<jobject>::Adopt(env, env->NewObject(arrayListClass.obj(), ctor));
    for (const auto& value : values) {
        jobject element = convElemJ(env, value);
        env->CallBooleanMethod(list.obj(), addMethod, element);
        env->DeleteLocalRef(element);
    }
    return list;
}

template <typename K, typename V>
webrtc::ScopedJavaLocalRef<jobject> parseJMap(JNIEnv* env, const std::map<K, V>& values) {
    const auto hashMapClass = findClass(env, "java/util/HashMap");
    const jmethodID ctor = env->GetMethodID(hashMapClass.obj(), "<init>", "()V");
    const jmethodID putMethod = env->GetMethodID(hashMapClass.obj(), "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    auto map = webrtc::ScopedJavaLocalRef<jobject>::Adopt(env, env->NewObject(hashMapClass.obj(), ctor));
    for (const auto& [key, value] : values) {
        jobject jKey = convElemJ(env, key);
        jobject jValue = convElemJ(env, value);
        env->CallObjectMethod(map.obj(), putMethod, jKey, jValue);
        env->DeleteLocalRef(jKey);
        env->DeleteLocalRef(jValue);
    }
    return map;
}

template <typename F>
auto parseOptional(JNIEnv* env, jobject value, F converter) -> std::optional<decltype(converter(value))> {
    return value != nullptr ? std::optional(converter(value)) : std::nullopt;
}

template <typename T, typename F>
jobject parseJOptional(JNIEnv* env, const std::optional<T>& value, F converter) {
    return value.has_value() ? converter(*value) : nullptr;
}

@for e in enums
@if e.emit
inline @{e.cpp} parse@{e.name}(JNIEnv* env, jobject value) {
    const auto clazz = webrtc::ScopedJavaLocalRef<jclass>::Adopt(env, env->GetObjectClass(value));
    const jmethodID ordinalMethod = env->GetMethodID(clazz.obj(), "ordinal", "()I");
    switch (env->CallIntMethod(value, ordinalMethod)) {
@for mem in e.members
        case @{mem.ord}: return @{e.cpp}::@{mem.cpp};
@end
        default: return {};
    }
}

inline webrtc::ScopedJavaLocalRef<jobject> parseJ@{e.name}(JNIEnv* env, @{e.cpp} value) {
@if e.ns
    const auto clazz = findClass(env, "@{config.java_pkg|slash}/@{e.ns}/@{e.name}");
@else
    const auto clazz = findClass(env, "@{config.java_pkg|slash}/@{e.name}");
@end
@if e.ns
    const jmethodID valuesMethod = env->GetStaticMethodID(clazz.obj(), "values", "()[L@{config.java_pkg|slash}/@{e.ns}/@{e.name};");
@else
    const jmethodID valuesMethod = env->GetStaticMethodID(clazz.obj(), "values", "()[L@{config.java_pkg|slash}/@{e.name};");
@end
    const auto values = webrtc::ScopedJavaLocalRef<jobjectArray>::Adopt(env, static_cast<jobjectArray>(env->CallStaticObjectMethod(clazz.obj(), valuesMethod)));
    jint ordinal = 0;
    switch (value) {
@for mem in e.members
        case @{e.cpp}::@{mem.cpp}: ordinal = @{mem.ord}; break;
@end
    }
    return webrtc::ScopedJavaLocalRef<jobject>::Adopt(env, env->GetObjectArrayElement(values.obj(), ordinal));
}
@end
@end
@for s in structs
inline @{s.cpp} parse@{s.name}(JNIEnv* env, jobject value) {
    const auto clazz = webrtc::ScopedJavaLocalRef<jclass>::Adopt(env, env->GetObjectClass(value));
    return @{s.cpp}(
@for f in s.fields
@if f.scalar
        static_cast<@{f.cpptype}>(env->@{f.type|conv#jget#f.type}(value, env->GetFieldID(clazz.obj(), "@{f.name}", "@{f.type|conv#jnidesc#f.type}")))@{f.sep}
@else
@if f.optional
        parseOptional(env, env->GetObjectField(value, env->GetFieldID(clazz.obj(), "@{f.name}", "@{f.type|conv#jnidesc#f.type}")), [env](jobject o) {
@if f.bytes
            return parseByteArray(env, static_cast<jbyteArray>(o));
@else
@if f.string
            return parseString(env, static_cast<jstring>(o));
@else
@if f.vector
            return parseList<@{f.elcpp}>(env, o);
@else
            return parse@{f.type|base}(env, o);
@end
@end
@end
        })@{f.sep}
@else
@if f.bytes
        parseByteArray(env, static_cast<jbyteArray>(env->GetObjectField(value, env->GetFieldID(clazz.obj(), "@{f.name}", "[B"))))@{f.sep}
@else
@if f.string
        parseString(env, static_cast<jstring>(env->GetObjectField(value, env->GetFieldID(clazz.obj(), "@{f.name}", "Ljava/lang/String;"))))@{f.sep}
@else
@if f.vector
        parseList<@{f.elcpp}>(env, env->GetObjectField(value, env->GetFieldID(clazz.obj(), "@{f.name}", "Ljava/util/List;")))@{f.sep}
@else
        parse@{f.type|base}(env, env->GetObjectField(value, env->GetFieldID(clazz.obj(), "@{f.name}", "@{f.type|conv#jnidesc#f.type}")))@{f.sep}
@end
@end
@end
@end
@end
@end
    );
}

inline webrtc::ScopedJavaLocalRef<jobject> parseJ@{s.name}(JNIEnv* env, const @{s.cpp}& value) {
@if s.ns
    const auto clazz = findClass(env, "@{config.java_pkg|slash}/@{s.ns}/@{s.name}");
@else
    const auto clazz = findClass(env, "@{config.java_pkg|slash}/@{s.name}");
@end
    const jmethodID ctor = env->GetMethodID(clazz.obj(), "<init>", "("
@for f in s.fields
        "@{f.type|conv#jnidesc#f.type}"
@end
        ")V");
    return webrtc::ScopedJavaLocalRef<jobject>::Adopt(env, env->NewObject(clazz.obj(), ctor
@for f in s.fields
@if f.scalar
        , static_cast<@{f.type|conv#jcast#f.type}>(value.@{f.cpp})
@else
@if f.optional
        , parseJOptional(env, value.@{f.cpp}, [env](const auto& o) -> jobject {
@if f.bytes
            return parseJByteArray(env, o).Release();
@else
@if f.string
            return parseJString(env, o).Release();
@else
@if f.vector
            return parseJList(env, o).Release();
@else
            return parseJ@{f.type|base}(env, o).Release();
@end
@end
@end
        })
@else
@if f.bytes
        , parseJByteArray(env, value.@{f.cpp}).Release()
@else
@if f.string
        , parseJString(env, value.@{f.cpp}).Release()
@else
@if f.vector
        , parseJList(env, value.@{f.cpp}).Release()
@else
        , parseJ@{f.type|base}(env, value.@{f.cpp}).Release()
@end
@end
@end
@end
@end
@end
    ));
}
@end
