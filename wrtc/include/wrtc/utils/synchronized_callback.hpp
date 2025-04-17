//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <functional>
#include <mutex>

namespace wrtc {

    template <typename... Args> class
    synchronized_callback final {
        std::function<void(Args...)> callback;
        mutable std::mutex mutex;

    public:
        synchronized_callback() = default;

        ~synchronized_callback() { *this = nullptr; }

        synchronized_callback &operator=(std::function<void(Args...)> func) {
            std::lock_guard lock(mutex);
            callback = std::move(func);
            return *this;
        }

        bool operator()(Args... args) const {
            std::lock_guard lock(mutex);
            if (!callback)
                return false;
            callback(std::move(args)...);
            return true;
        }
    };

    template <> class
    synchronized_callback<void> final {
        std::function<void()> callback;
        mutable std::mutex mutex;

    public:
        synchronized_callback() = default;

        ~synchronized_callback() { *this = nullptr; }

        synchronized_callback &operator=(std::function<void()> func) {
            std::lock_guard lock(mutex);
            callback = std::move(func);
            return *this;
        }

        bool operator()() const {
            std::lock_guard lock(mutex);
            if (!callback)
                return false;
            callback();
            return true;
        }
    };

} // wrtc
