//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <functional>
#include <mutex>

namespace wrtc {

    template <typename... Args> class synchronized_callback {
    public:
        synchronized_callback() = default;

        virtual ~synchronized_callback() { *this = nullptr; }

        synchronized_callback &operator=(std::function<void(Args...)> func) {
            std::lock_guard lock(mutex);
            set(std::move(func));
            return *this;
        }

        bool operator()(Args... args) const {
            std::lock_guard lock(mutex);
            return call(std::move(args)...);
        }

    protected:
        virtual void set(std::function<void(Args...)> func) { callback = std::move(func); }
        virtual bool call(Args... args) const {
            if (!callback)
                return false;

            callback(std::move(args)...);
            return true;
        }

        std::function<void(Args...)> callback;
        mutable std::recursive_mutex mutex;
    };

} // wrtc
