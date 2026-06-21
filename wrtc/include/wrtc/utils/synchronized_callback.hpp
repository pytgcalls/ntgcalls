//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <functional>
#include <mutex>
#include <rtc_base/logging.h>

namespace wrtc {
    template <typename Signature>
    class synchronized_callback;

    template <typename R, typename... Args>
    class synchronized_callback<R(Args...)> final {
        std::function<R(Args...)> callback;
        mutable std::mutex mutex;

    public:
        synchronized_callback() = default;
        ~synchronized_callback() { *this = nullptr; }

        synchronized_callback &operator=(std::function<R(Args...)> func) {
            std::lock_guard lock(mutex);
            callback = std::move(func);
            return *this;
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator bool() const {
            std::lock_guard lock(mutex);
            return static_cast<bool>(callback);
        }

        auto operator()(Args... args) const {
            std::lock_guard lock(mutex);
            if constexpr (std::is_void_v<R>) {
                if (!callback) return false;
                try {
                    callback(std::move(args)...);
                } catch (const std::exception& e) {
                    RTC_LOG(LS_ERROR) << "synchronized_callback threw an exception: " << e.what();
                } catch (...) {
                    RTC_LOG(LS_ERROR) << "synchronized_callback threw an unknown exception";
                }
                return true;
            } else {
                if (!callback) return std::optional<R>{std::nullopt};
                try {
                    return std::optional<R>{callback(std::move(args)...)};
                } catch (const std::exception& e) {
                    RTC_LOG(LS_ERROR) << "synchronized_callback threw an exception: " << e.what();
                } catch (...) {
                    RTC_LOG(LS_ERROR) << "synchronized_callback threw an unknown exception";
                }
                return std::optional<R>{std::nullopt};
            }
        }
    };

} // wrtc
