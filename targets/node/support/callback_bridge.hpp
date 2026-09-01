#pragma once

#include <functional>
#include <utility>
#include <vector>

#include <napi.h>

template<typename Payload, typename Convert>
class CallbackBridge final {
public:
    CallbackBridge(const Napi::Env env, const Napi::Function& fn, Convert convert): convert_(std::move(convert)) {
        tsfn_ = Napi::ThreadSafeFunction::New(env, fn, "ntg_callback", 0, 1);
        tsfn_.Unref(env);
    }

    ~CallbackBridge() {
        tsfn_.Release();
    }

    CallbackBridge(const CallbackBridge&) = delete;
    CallbackBridge& operator=(const CallbackBridge&) = delete;

    void emit(Payload* data) {
        tsfn_.NonBlockingCall(data, [conv = convert_](const Napi::Env env, const Napi::Function fn, Payload* payload) {
            std::vector<napi_value> args = conv(env, payload);
            fn.Call(args);
            delete payload;
        });
    }

private:
    Napi::ThreadSafeFunction tsfn_;
    Convert convert_;
};
