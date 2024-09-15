#include "utils.hpp"

ntgcalls::NTgCalls* getInstance(JNIEnv *env, jobject obj) {
    jclass clazz = env->GetObjectClass(obj);
    jlong ptr = env->GetLongField(obj,  env->GetFieldID(clazz, "nativePointer", "J"));
    if (ptr != 0) {
        return reinterpret_cast<ntgcalls::NTgCalls*>(ptr);
    }
    return nullptr;
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

    return {
        parseInputMode(env, inputMode),
        sampleRate,
        bitsPerSample,
        channelCount,
        std::string(env->GetStringUTFChars(input, nullptr))
    };
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

    return {
        parseInputMode(env, inputMode),
        width,
        height,
        fps,
        std::string(env->GetStringUTFChars(input, nullptr))
    };
}

ntgcalls::MediaDescription parseMediaDescription(JNIEnv *env, jobject mediaDescription) {
    jclass mediaDescriptionClass = env->GetObjectClass(mediaDescription);
    jfieldID audioField = env->GetFieldID(mediaDescriptionClass, "audio", "Lorg/pytgcalls/ntgcalls/media/AudioDescription;");
    jfieldID videoField = env->GetFieldID(mediaDescriptionClass, "video", "Lorg/pytgcalls/ntgcalls/media/VideoDescription;");
    auto audio = env->GetObjectField(mediaDescription, audioField);
    auto video = env->GetObjectField(mediaDescription, videoField);
    return {
        audio != nullptr ? std::optional(parseAudioDescription(env, audio)) : std::nullopt,
        video != nullptr ? std::optional(parseVideoDescription(env, video)) : std::nullopt
    };
}

ntgcalls::BaseMediaDescription::InputMode parseInputMode(JNIEnv *env, jint inputMode) {
    ntgcalls::BaseMediaDescription::InputMode res;
    if (auto check = ntgcalls::BaseMediaDescription::InputMode::File;inputMode == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::InputMode::Shell;inputMode == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::InputMode::FFmpeg;inputMode == check) {
        res |= check;
    }
    if (auto check = ntgcalls::BaseMediaDescription::InputMode::NoLatency;inputMode == check) {
        res |= check;
    }
    return res;
}

void throwJavaException(JNIEnv *env, std::string name, std::string message) {
    if (name == "RuntimeException") {
        name = "java/lang/" + name;
    } else {
        name = "org/pytgcalls/ntgcalls/exceptions/" + name;
    }
    jclass exceptionClass = env->FindClass(name.c_str());
    if (exceptionClass != nullptr) {
        env->ThrowNew(exceptionClass, message.c_str());
    }
}
