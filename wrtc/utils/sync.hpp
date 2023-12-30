//
// Created by Laky64 on 11/08/2023.
//

#pragma once

#include <future>
#include <optional>

namespace wrtc {

    template <class T>
    class Sync {
        std::promise<T> promise{};

    public:
        const std::function<void(T)> onSuccess = [this](T value) {
            promise.set_value(value);
        };

        const std::function<void(const std::exception&)> onFailed = [this](const std::exception& value) {
            promise.set_exception(std::make_exception_ptr(value));
        };

        T get() {
            return promise.get_future().get();
        }
    };

    template <class T>
    class Sync<std::optional<T>> {
        std::promise<std::optional<T>> promise{};

    public:
        const std::function<void(T)> onSuccess = [this](T value) {
            promise.set_value(std::move(value));
        };

        const std::function<void(const std::exception&)> onFailed = [this](const std::exception& value) {
            promise.set_exception(std::make_exception_ptr(value));
        };

        T get() {
            return promise.get_future().get().value();
        }
    };

    template<> class
    Sync<void> {
        std::promise<void> promise{};

    public:
        const std::function<void()> onSuccess = [this] {
            promise.set_value();
        };

        const std::function<void(const std::exception&)> onFailed = [this](const std::exception& value) {
            promise.set_exception(std::make_exception_ptr(value));
        };

        void wait() {
            promise.get_future().get();
        }
    };
}
