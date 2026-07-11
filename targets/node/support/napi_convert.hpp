//
// Node-API leaf converters (hand-written, target-level).
// Only the type-agnostic primitives live here; enum/struct/vector/map/optional
// converters are emitted in the generated addon so name lookup resolves them.
//
#pragma once

#include <cstdint>
#include <string>

#include <napi.h>

#include <wrtc/utils/binary.hpp>

inline Napi::Value to_js(const Napi::Env env, const bool v) { return Napi::Boolean::New(env, v); }
inline Napi::Value to_js(const Napi::Env env, const double v) { return Napi::Number::New(env, v); }
inline Napi::Value to_js(const Napi::Env env, const int8_t v) { return Napi::Number::New(env, static_cast<int32_t>(v)); }
inline Napi::Value to_js(const Napi::Env env, const uint8_t v) { return Napi::Number::New(env, static_cast<uint32_t>(v)); }
inline Napi::Value to_js(const Napi::Env env, const int16_t v) { return Napi::Number::New(env, static_cast<int32_t>(v)); }
inline Napi::Value to_js(const Napi::Env env, const uint16_t v) { return Napi::Number::New(env, static_cast<uint32_t>(v)); }
inline Napi::Value to_js(const Napi::Env env, const int32_t v) { return Napi::Number::New(env, v); }
inline Napi::Value to_js(const Napi::Env env, const uint32_t v) { return Napi::Number::New(env, v); }
inline Napi::Value to_js(const Napi::Env env, const int64_t v) { return Napi::BigInt::New(env, v); }
inline Napi::Value to_js(const Napi::Env env, const uint64_t v) { return Napi::BigInt::New(env, v); }
inline Napi::Value to_js(const Napi::Env env, const std::string& v) { return Napi::String::New(env, v); }

inline Napi::Value to_js(const Napi::Env env, const bytes::binary& v) {
    return Napi::Buffer<uint8_t>::Copy(env, v.data(), v.size());
}

inline void from_js(const Napi::Value& v, bool& out) { out = v.As<Napi::Boolean>(); }
inline void from_js(const Napi::Value& v, double& out) { out = v.As<Napi::Number>().DoubleValue(); }
inline void from_js(const Napi::Value& v, int8_t& out) { out = static_cast<int8_t>(v.As<Napi::Number>().Int32Value()); }
inline void from_js(const Napi::Value& v, uint8_t& out) { out = static_cast<uint8_t>(v.As<Napi::Number>().Uint32Value()); }
inline void from_js(const Napi::Value& v, int16_t& out) { out = static_cast<int16_t>(v.As<Napi::Number>().Int32Value()); }
inline void from_js(const Napi::Value& v, uint16_t& out) { out = static_cast<uint16_t>(v.As<Napi::Number>().Uint32Value()); }
inline void from_js(const Napi::Value& v, int32_t& out) { out = v.As<Napi::Number>().Int32Value(); }
inline void from_js(const Napi::Value& v, uint32_t& out) { out = v.As<Napi::Number>().Uint32Value(); }
inline void from_js(const Napi::Value& v, std::string& out) { out = v.As<Napi::String>().Utf8Value(); }

inline void from_js(const Napi::Value& v, int64_t& out) {
    bool lossless;
    out = v.As<Napi::BigInt>().Int64Value(&lossless);
}

inline void from_js(const Napi::Value& v, uint64_t& out) {
    bool lossless;
    out = v.As<Napi::BigInt>().Uint64Value(&lossless);
}

inline void from_js(const Napi::Value& v, bytes::binary& out) {
    const auto buf = v.As<Napi::Buffer<uint8_t>>();
    out = bytes::binary(buf.Data(), buf.Data() + buf.Length());
}
