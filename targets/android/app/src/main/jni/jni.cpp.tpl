@config java_pkg = io.github.pytgcalls
@typemap long = jlong
@typemap ulong = jlong
@typemap int = jint
@typemap uint = jint
@typemap int8 = jint
@typemap uint8 = jint
@typemap int16 = jint
@typemap uint16 = jint
@typemap bool = jboolean
@typemap double = jdouble
@typemap bytes = jbyteArray
@typemap string = jstring
@typemap Void = void
@typemap Vector<*> = jobject
@typemap Map<*, *> = jobject
@typemap * = jobject
@wrapmap argconv long = static_cast<int64_t>($)
@wrapmap argconv ulong = static_cast<uint64_t>($)
@wrapmap argconv int = static_cast<int32_t>($)
@wrapmap argconv uint = static_cast<uint32_t>($)
@wrapmap argconv int8 = static_cast<int8_t>($)
@wrapmap argconv uint8 = static_cast<uint8_t>($)
@wrapmap argconv int16 = static_cast<int16_t>($)
@wrapmap argconv uint16 = static_cast<uint16_t>($)
@wrapmap argconv bool = static_cast<bool>($)
@wrapmap argconv double = static_cast<double>($)
@wrapmap argconv bytes = parseByteArray(env, $)
@wrapmap argconv string = parseString(env, $)
@wrapmap argconv Vector<*> = parseList(env, $)
@wrapmap argconv * = parse#(env, $)
@wrapmap retopen long = static_cast<jlong>(
@wrapmap retopen ulong = static_cast<jlong>(
@wrapmap retopen int = static_cast<jint>(
@wrapmap retopen uint = static_cast<jint>(
@wrapmap retopen bool = static_cast<jboolean>(
@wrapmap retopen double = static_cast<jdouble>(
@wrapmap retopen bytes = parseJByteArray(env,
@wrapmap retopen string = parseJString(env,
@wrapmap retopen Vector<*> = parseJList(env,
@wrapmap retopen Map<*, *> = parseJMap(env,
@wrapmap retopen * = parseJ#(env,
@wrapmap retclose long = )
@wrapmap retclose ulong = )
@wrapmap retclose int = )
@wrapmap retclose uint = )
@wrapmap retclose bool = )
@wrapmap retclose double = )
@wrapmap retclose * = ).Release()
@wrapmap default long = 0
@wrapmap default ulong = 0
@wrapmap default int = 0
@wrapmap default uint = 0
@wrapmap default bool = static_cast<jboolean>(false)
@wrapmap default double = 0
@wrapmap default * = nullptr
@wrapmap cb2jni long = static_cast<jlong>($)
@wrapmap cb2jni ulong = static_cast<jlong>($)
@wrapmap cb2jni int = static_cast<jint>($)
@wrapmap cb2jni uint = static_cast<jint>($)
@wrapmap cb2jni int8 = static_cast<jint>($)
@wrapmap cb2jni uint8 = static_cast<jint>($)
@wrapmap cb2jni int16 = static_cast<jint>($)
@wrapmap cb2jni uint16 = static_cast<jint>($)
@wrapmap cb2jni bool = static_cast<jboolean>($)
@wrapmap cb2jni double = static_cast<jdouble>($)
@wrapmap cb2jni bytes = parseJByteArray(env, $).Release()
@wrapmap cb2jni string = parseJString(env, $).Release()
@wrapmap cb2jni Vector<*> = parseJList(env, $).Release()
@wrapmap cb2jni * = parseJ#(env, $).Release()
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
@wrapmap jnidesc * = Lio/github/pytgcalls/@;
#include "utils.hpp"

#define HANDLE_EXCEPTIONS \
@for e in excs
@if e.parent
    catch (const @{e.cpp}& e) { throwJavaException(env, "@{config.java_pkg|slash}/exceptions/@{e.name}Exception", e.what()); } \
@end
@end
@for e in excs
@if e.parent
@else
    catch (const @{e.cpp}& e) { throwJavaException(env, "@{config.java_pkg|slash}/exceptions/@{e.name}Exception", e.what()); } \
@end
@end
    catch (const std::exception& e) { throwJavaException(env, "java/lang/RuntimeException", e.what()); } \
    catch (...) { throwJavaException(env, "java/lang/RuntimeException", "Unknown error"); }

struct InstanceCallbacks {
@for cb in callbacks
    std::optional<JavaCallback> @{cb.method};
@end
};
std::map<jlong, InstanceCallbacks> callbacksInstances;
std::mutex callbacksMutex;

@for c in classes
extern "C"
JNIEXPORT void JNICALL Java_io_github_pytgcalls_@{c.name}_init(JNIEnv *env, jobject thiz) {
    auto clazz = webrtc::ScopedJavaLocalRef<jclass>::Adopt(env, env->GetObjectClass(thiz));
    jfieldID fid = env->GetFieldID(clazz.obj(), "nativePointer", "J");
    auto instancePtr = reinterpret_cast<jlong>(new @{c.cpp}());
    env->SetLongField(thiz, fid, instancePtr);
    callbacksInstances[instancePtr] = InstanceCallbacks();
    auto instance = reinterpret_cast<@{c.cpp}*>(instancePtr);
@for cb in callbacks
    instance->@{cb.method}([instancePtr](
@for a in cb.cbargs
    @{a.cpptype} @{a.name|camel}@{a.sep}
@end
    ) {
        std::lock_guard lock(callbacksMutex);
        auto cb = callbacksInstances[instancePtr].@{cb.method};
        if (!cb) {
            return;
        }
        auto env = (JNIEnv*) wrtc::utils::GetJNIEnv();
        env->CallVoidMethod(cb->callback, cb->methodId
@for a in cb.cbargs
                , @{a.name|camel|conv#cb2jni#a.type}
@end
        );
        CAPTURE_JAVA_EXCEPTION
    });
@end
}

extern "C"
JNIEXPORT void JNICALL Java_io_github_pytgcalls_@{c.name}_destroy(JNIEnv *env, jobject thiz) {
    auto clazz = webrtc::ScopedJavaLocalRef<jclass>::Adopt(env, env->GetObjectClass(thiz));
    jfieldID fid = env->GetFieldID(clazz.obj(), "nativePointer", "J");
    jlong ptr = env->GetLongField(thiz, fid);
    if (ptr != 0) {
        delete reinterpret_cast<@{c.cpp}*>(ptr);
        env->SetLongField(thiz, fid, 0);
    }
}

@for m in c.methods
@if m.iscb
@else
@skip m.name in enableGlibLoop
extern "C"
JNIEXPORT @{m.ret|type} JNICALL Java_io_github_pytgcalls_@{c.name}_@{m.name}(
    JNIEnv *env,
@if m.static
@if m.params
    jclass clazz,
@else
    jclass clazz
@end
@else
@if m.params
    jobject thiz,
@else
    jobject thiz
@end
@end
@for p in m.params
    @{p.type|type} @{p.name|camel}@{p.sep}
@end
) {
    try {
@if m.static
@else
        auto instance = getInstance(env, thiz);
@end
@if m.isvoid
@else
        return @{m.ret|conv#retopen#m.ret}
@end
@if m.static
        @{c.cpp}::@{m.name|snake}(
@else
        instance->@{m.name|snake}(
@end
@for p in m.params
@if p.vector
                parseList<@{p.elcpp}>(env, @{p.name|camel})@{p.sep}
@else
                @{p.name|camel|conv#argconv#p.type}@{p.sep}
@end
@end
@if m.isvoid
        );
@else
        )@{m.ret|conv#retclose#m.ret};
@end
    } HANDLE_EXCEPTIONS
@if m.isvoid
@else
    return @{m.ret|conv#default#m.ret};
@end
}

@end
@end
@for cb in callbacks
extern "C"
JNIEXPORT void JNICALL Java_io_github_pytgcalls_@{c.name}_@{cb.method|camel}(JNIEnv *env, jobject thiz, jobject callback) {
    std::lock_guard lock(callbacksMutex);
    auto instancePtr = getInstancePtr(env, thiz);
    if (auto check = callbacksInstances[instancePtr].@{cb.method}) {
        env->DeleteGlobalRef(check->callback);
    }
    auto callbackClass = env->GetObjectClass(callback);
    if (callbackClass == nullptr) {
        return;
    }
    callbacksInstances[instancePtr].@{cb.method} = JavaCallback{
        env->NewGlobalRef(callback),
        env->GetMethodID(callbackClass, "@{cb.method|camel}",
            "("
@for a in cb.cbargs
            "@{a.type|conv#jnidesc#a.type}"
@end
            ")V")
    };
    env->DeleteLocalRef(callbackClass);
}

@end
@end