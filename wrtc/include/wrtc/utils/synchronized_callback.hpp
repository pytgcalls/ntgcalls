//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <functional>
#include <memory>
#include <mutex>

namespace wrtc {

    template <typename... Args> class
    synchronized_callback final {
        std::shared_ptr<std::function<void(Args...)>> callback;
        mutable std::mutex mutex;

    public:
        synchronized_callback() = default;

        ~synchronized_callback() { *this = nullptr; }

        synchronized_callback &operator=(std::function<void(Args...)> func) {
            auto next = func ? std::make_shared<std::function<void(Args...)>>(std::move(func)) : nullptr;
            std::lock_guard lock(mutex);
            callback = std::move(next);
            return *this;
        }

        bool operator()(Args... args) const {
            std::shared_ptr<std::function<void(Args...)>> snapshot;
            {
                std::lock_guard lock(mutex);
                snapshot = callback;
            }
            if (!snapshot || !*snapshot)
                return false;
            (*snapshot)(std::move(args)...);
            return true;
        }
    };

    template <> class
    synchronized_callback<void> final {
        std::shared_ptr<std::function<void()>> callback;
        mutable std::mutex mutex;

    public:
        synchronized_callback() = default;

        ~synchronized_callback() { *this = nullptr; }

        synchronized_callback &operator=(std::function<void()> func) {
            auto next = func ? std::make_shared<std::function<void()>>(std::move(func)) : nullptr;
            std::lock_guard lock(mutex);
            callback = std::move(next);
            return *this;
        }

        bool operator()() const {
            std::shared_ptr<std::function<void()>> snapshot;
            {
                std::lock_guard lock(mutex);
                snapshot = callback;
            }
            if (!snapshot || !*snapshot)
                return false;
            (*snapshot)();
            return true;
        }
    };

} // wrtc
