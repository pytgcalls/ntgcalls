//
// Created by Lauren on 24/06/26.
//

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <ranges>
#include <unordered_map>

namespace ntgcalls::utils {

    class ShutdownHook {
        std::mutex mutex_;
        std::unordered_map<uint64_t, std::function<void()>> hooks_;
        uint64_t next_token_ = 0;

        static ShutdownHook& instance() {
            static ShutdownHook hook;
            return hook;
        }

    public:
        static uint64_t add(std::function<void()> hook) {
            auto& self = instance();
            std::lock_guard lock(self.mutex_);
            const auto token = ++self.next_token_;
            self.hooks_[token] = std::move(hook);
            return token;
        }

        static void runAll() {
            auto& self = instance();
            std::lock_guard lock(self.mutex_);
            for (const auto& hook : self.hooks_ | std::views::values) {
                hook();
            }
        }

        static void remove(const uint64_t token) {
            auto& self = instance();
            std::lock_guard lock(self.mutex_);
            self.hooks_.erase(token);
        }
    };

} // ntgcalls::utils
