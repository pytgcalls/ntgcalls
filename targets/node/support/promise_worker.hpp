//
// Async bridge: runs a blocking C++ call on the libuv pool and resolves a JS
// Promise. Convert runs on the JS thread in OnOK; it is supplied by the addon
// so name lookup for to_js resolves the generated overloads.
//
#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include <napi.h>

template<typename Result, typename Compute, typename Convert>
class AsyncRunner final : public Napi::AsyncWorker {
public:
    AsyncRunner(const Napi::Env env, Compute compute, Convert convert)
        : AsyncWorker(env),
          deferred_(Napi::Promise::Deferred::New(env)),
          compute_(std::move(compute)),
          convert_(std::move(convert)) {}

    Napi::Promise Promise() { return deferred_.Promise(); }

    void Execute() override {
        try {
            result_ = compute_();
        } catch (const std::exception& e) {
            SetError(e.what());
        }
    }

    void OnOK() override { deferred_.Resolve(convert_(Env(), result_)); }
    void OnError(const Napi::Error& e) override { deferred_.Reject(e.Value()); }

private:
    Napi::Promise::Deferred deferred_;
    Compute compute_;
    Convert convert_;
    Result result_{};
};

template<typename Compute>
class AsyncRunnerVoid final : public Napi::AsyncWorker {
public:
    AsyncRunnerVoid(const Napi::Env env, Compute compute)
        : AsyncWorker(env),
          deferred_(Napi::Promise::Deferred::New(env)),
          compute_(std::move(compute)) {}

    Napi::Promise Promise() { return deferred_.Promise(); }

    void Execute() override {
        try {
            compute_();
        } catch (const std::exception& e) {
            SetError(e.what());
        }
    }

    void OnOK() override { deferred_.Resolve(Env().Undefined()); }
    void OnError(const Napi::Error& e) override { deferred_.Reject(e.Value()); }

private:
    Napi::Promise::Deferred deferred_;
    Compute compute_;
};

template<typename Compute, typename Convert>
Napi::Promise run_async(const Napi::Env env, Compute compute, Convert convert) {
    using Result = std::invoke_result_t<Compute>;
    auto* worker = new AsyncRunner<Result, Compute, Convert>(env, std::move(compute), std::move(convert));
    worker->Queue();
    return worker->Promise();
}

template<typename Compute>
Napi::Promise run_async_void(const Napi::Env env, Compute compute) {
    auto* worker = new AsyncRunnerVoid<Compute>(env, std::move(compute));
    worker->Queue();
    return worker->Promise();
}
