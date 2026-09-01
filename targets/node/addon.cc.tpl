@out @{config.self_dir}/addon.cc
#include <map>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include <napi.h>

#include <napi_convert.hpp>
#include <promise_worker.hpp>
#include <callback_bridge.hpp>
#include <napi_log.hpp>

#include <ntgcalls/ntgcalls.hpp>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/utils/shutdown_hook.hpp>

@for e in enums
@if e.emit
inline Napi::Value to_js(const Napi::Env env, const @{e.cpp} value) { return Napi::Number::New(env, static_cast<int32_t>(value)); }
inline void from_js(const Napi::Value& v, @{e.cpp}& out) { out = static_cast<@{e.cpp}>(v.As<Napi::Number>().Int32Value()); }
@end
@end

// Reader: builds a C++ value from JS. Structs are constructed through their
// constructor (specializations below) so no default constructor is required.
template<typename T>
struct JsReader {
    static T read(const Napi::Value& v) {
        T value{};
        from_js(v, value);
        return value;
    }
};

template<typename T>
struct JsReader<std::optional<T>> {
    static std::optional<T> read(const Napi::Value& v) {
        if (v.IsNull() || v.IsUndefined()) {
            return std::nullopt;
        }
        return JsReader<T>::read(v);
    }
};

template<typename T>
struct JsReader<std::vector<T>> {
    static std::vector<T> read(const Napi::Value& v) {
        const auto arr = v.As<Napi::Array>();
        std::vector<T> out;
        out.reserve(arr.Length());
        for (uint32_t i = 0; i < arr.Length(); ++i) {
            out.push_back(JsReader<T>::read(arr.Get(i)));
        }
        return out;
    }
};

@for s in structs
Napi::Value to_js(Napi::Env env, const @{s.cpp}& value);
@end

template<typename T>
Napi::Value to_js(const Napi::Env env, const std::optional<T>& value) {
    if (!value.has_value()) {
        return env.Null();
    }
    return to_js(env, *value);
}

template<typename T>
Napi::Value to_js(const Napi::Env env, const std::vector<T>& value) {
    auto arr = Napi::Array::New(env, value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        arr.Set(static_cast<uint32_t>(i), to_js(env, value[i]));
    }
    return arr;
}

template<typename K, typename V>
Napi::Value to_js(const Napi::Env env, const std::map<K, V>& value) {
    const auto ctor = env.Global().Get("Map").As<Napi::Function>();
    auto js = ctor.New({});
    const auto setter = js.Get("set").As<Napi::Function>();
    for (const auto& [key, val] : value) {
        setter.Call(js, { to_js(env, key), to_js(env, val) });
    }
    return js;
}

@for s in structs
Napi::Value to_js(const Napi::Env env, const @{s.cpp}& value) {
    auto obj = Napi::Object::New(env);
@for f in s.fields
@if f.intenum
    obj.Set("@{f.name|camel}", to_js(env, static_cast<int32_t>(value.@{f.cpp})));
@else
    obj.Set("@{f.name|camel}", to_js(env, value.@{f.cpp}));
@end
@end
    return obj;
}

template<>
struct JsReader<@{s.cpp}> {
    static @{s.cpp} read(const Napi::Value& v) {
        const auto obj = v.As<Napi::Object>();
        return @{s.cpp}(
@for f in s.fields
@if f.bytes
@if f.optional
            JsReader<std::optional<bytes::binary>>::read(obj.Get("@{f.name|camel}"))@{f.sep}
@else
            JsReader<bytes::binary>::read(obj.Get("@{f.name|camel}"))@{f.sep}
@end
@else
@if f.optional
            JsReader<std::optional<@{f.cpptype}>>::read(obj.Get("@{f.name|camel}"))@{f.sep}
@else
            JsReader<@{f.cpptype}>::read(obj.Get("@{f.name|camel}"))@{f.sep}
@end
@end
@end
        );
    }
};

@end
class NapiArgs final {
public:
    explicit NapiArgs(const Napi::CallbackInfo& info) : info_(info) {}

    template<typename T>
    T next() { return JsReader<T>::read(info_[index_++]); }

private:
    const Napi::CallbackInfo& info_;
    size_t index_ = 0;
};

struct JsConvert final {
    template<typename T>
    Napi::Value operator()(const Napi::Env env, T& value) const { return to_js(env, value); }
};

@for c in classes
class @{c.name} final : public Napi::ObjectWrap<@{c.name}> {
public:
    static Napi::Object Init(const Napi::Env env, Napi::Object exports) {
        const Napi::Function ctor = DefineClass(env, "@{c.name}", {
@for m in c.methods
@if m.iscb
@else
@if m.static
            StaticMethod<&@{c.name}::@{m.name|pascal}>("@{m.name|camel}"),
@else
            InstanceMethod<&@{c.name}::@{m.name|pascal}>("@{m.name|camel}"),
@end
@end
@end
@for cb in callbacks
            InstanceMethod<&@{c.name}::@{cb.method|pascal}>("@{cb.method|camel}"),
@end
        });
        exports.Set("@{c.name}", ctor);
        return exports;
    }

    explicit @{c.name}(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<@{c.name}>(info), impl_(std::make_unique<@{c.cpp}>()) {}

private:
    std::unique_ptr<@{c.cpp}> impl_;

@for m in c.methods
@if m.iscb
@else
@if m.static
    static Napi::Value @{m.name|pascal}(const Napi::CallbackInfo& info) {
@else
    Napi::Value @{m.name|pascal}(const Napi::CallbackInfo& info) {
@end
        NapiArgs args(info);
@for a in m.args
@if a.bytes
@if a.optional
        auto @{a.name|snake} = args.next<std::optional<bytes::binary>>();
@else
        auto @{a.name|snake} = args.next<bytes::binary>();
@end
@else
        auto @{a.name|snake} = args.next<@{a.cpptype}>();
@end
@end
@if m.async
        auto* impl = impl_.get();
@if m.isvoid
        return run_async_void(info.Env(), [impl@{m.argsep}
@for a in m.args
            @{a.name|snake}@{a.sep}
@end
        ] {
            impl->@{m.name|snake}(
@for a in m.args
                @{a.name|snake}@{a.sep}
@end
            );
        });
@else
        return run_async(info.Env(), [impl@{m.argsep}
@for a in m.args
            @{a.name|snake}@{a.sep}
@end
        ] {
            return impl->@{m.name|snake}(
@for a in m.args
                @{a.name|snake}@{a.sep}
@end
            );
        }, JsConvert{});
@end
@else
@if m.static
@if m.isvoid
        @{c.cpp}::@{m.name|snake}(
@for a in m.args
            @{a.name|snake}@{a.sep}
@end
        );
        return info.Env().Undefined();
@else
        return to_js(info.Env(), @{c.cpp}::@{m.name|snake}(
@for a in m.args
            @{a.name|snake}@{a.sep}
@end
        ));
@end
@else
@if m.isvoid
        impl_->@{m.name|snake}(
@for a in m.args
            @{a.name|snake}@{a.sep}
@end
        );
        return info.Env().Undefined();
@else
        return to_js(info.Env(), impl_->@{m.name|snake}(
@for a in m.args
            @{a.name|snake}@{a.sep}
@end
        ));
@end
@end
@end
    }

@end
@end
@for cb in callbacks
    Napi::Value @{cb.method|pascal}(const Napi::CallbackInfo& info) {
        struct Payload {
@for a in cb.cbargs
            std::remove_cvref_t<@{a.cpptype}> @{a.name|snake};
@end
        };
        auto convert = [](const Napi::Env env, Payload* p) -> std::vector<napi_value> {
            return {
@for a in cb.cbargs
                to_js(env, p->@{a.name|snake})@{a.sep}
@end
            };
        };
        auto bridge = std::make_shared<CallbackBridge<Payload, decltype(convert)>>(
            info.Env(), info[0].As<Napi::Function>(), convert);
        impl_->@{cb.method|snake}([bridge](
@for a in cb.cbargs
            @{a.cpptype} @{a.name|snake}@{a.sep}
@end
        ) {
            bridge->emit(new Payload{
@for a in cb.cbargs
                @{a.name|snake}@{a.sep}
@end
            });
        });
        return info.Env().Undefined();
    }

@end
};

@end
static Napi::Object InitModule(Napi::Env env, Napi::Object exports) {
    ntgcalls::support::install_log_bridge(env);
    env.AddCleanupHook([] {
        ntgcalls::utils::ShutdownHook::runAll();
    });
@for c in classes
    @{c.name}::Init(env, exports);
@end
@for e in enums
@if e.emit
    {
        auto values = Napi::Object::New(env);
@for mem in e.members
        values.Set("@{mem.disp}", Napi::Number::New(env, static_cast<int32_t>(@{e.cpp}::@{mem.cpp})));
@end
        exports.Set("@{e.name}", values);
    }
@end
@end
    exports.Set("setLogLevel", Napi::Function::New(env, [](const Napi::CallbackInfo& info) {
        ntgcalls::support::set_log_level(info[0].As<Napi::Number>().Int32Value());
        return info.Env().Undefined();
    }));
    {
        auto values = Napi::Object::New(env);
        values.Set("DEBUG", Napi::Number::New(env, static_cast<int32_t>(ntgcalls::utils::LogSink::Level::Debug)));
        values.Set("INFO", Napi::Number::New(env, static_cast<int32_t>(ntgcalls::utils::LogSink::Level::Info)));
        values.Set("WARNING", Napi::Number::New(env, static_cast<int32_t>(ntgcalls::utils::LogSink::Level::Warning)));
        values.Set("ERROR", Napi::Number::New(env, static_cast<int32_t>(ntgcalls::utils::LogSink::Level::Error)));
        values.Set("SILENT", Napi::Number::New(env, 16));
        exports.Set("LogLevel", values);
    }
    return exports;
}

NODE_API_MODULE(ntgcalls, InitModule)
