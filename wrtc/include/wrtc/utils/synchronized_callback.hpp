//
// Created by Lauren on 12/08/23.
//

#pragma once

#include <functional>
#include <mutex>
#include <rtc_base/logging.h>

namespace wrtc::utils {
    template<typename Signature>
    class synchronized_callback;

    template<typename R, typename... Args>
    class synchronized_callback<R(Args...)> final {
        std::function<R(Args...)> callback_;
        mutable std::mutex mutex_;

    public:
        synchronized_callback() = default;
        ~synchronized_callback() {
            *this = nullptr;
        }

        synchronized_callback& operator=(std::function<R(Args...)> func) {
            std::lock_guard lock(mutex_);
            callback_ = std::move(func);
            return *this;
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator bool() const {
            std::lock_guard lock(mutex_);
            return static_cast<bool>(callback_);
        }

        auto operator()(Args... args) const {
            std::lock_guard lock(mutex_);
            if constexpr (std::is_void_v<R>) {
                if (!callback_) return false;
                try {
                    callback_(std::move(args)...);
                } catch (const std::exception& e) {
                    RTC_LOG(LS_ERROR) << "synchronized_callback threw an exception: " << e.what();
                } catch (...) {
                    RTC_LOG(LS_ERROR) << "synchronized_callback threw an unknown exception";
                }
                return true;
            } else {
                if (!callback_) return std::optional<R>{std::nullopt};
                try {
                    return std::optional<R>{callback_(std::move(args)...)};
                } catch (const std::exception& e) {
                    RTC_LOG(LS_ERROR) << "synchronized_callback threw an exception: " << e.what();
                } catch (...) {
                    RTC_LOG(LS_ERROR) << "synchronized_callback threw an unknown exception";
                }
                return std::optional<R>{std::nullopt};
            }
        }
    };

} // wrtc::utils
