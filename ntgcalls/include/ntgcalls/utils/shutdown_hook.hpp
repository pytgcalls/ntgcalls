//
// Created by Laky64 on 24/06/2026.
//

#pragma once

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <ranges>
#include <unordered_map>
#include <ntgcalls/utils/binding_utils.hpp>

namespace ntgcalls {

    class ShutdownHook {
        std::mutex mutex;
        std::unordered_map<uint64_t, std::function<void()>> hooks;
        uint64_t nextToken = 0;
        bool registered = false;

        static ShutdownHook& instance() {
            static ShutdownHook hook;
            return hook;
        }

        static void runAll() {
            auto& self = instance();
            std::lock_guard lock(self.mutex);
            for (const auto& hook : self.hooks | std::views::values) {
                hook();
            }
        }

        void ensureRegistered() {
            if (registered) {
                return;
            }
            registered = true;
#ifdef PYTHON_ENABLED
            Py_AtExit(&ShutdownHook::runAll);
#elif IS_ANDROID
#else
            std::atexit(&ShutdownHook::runAll);
#endif
        }

    public:
        static uint64_t add(std::function<void()> hook) {
            auto& self = instance();
            std::lock_guard lock(self.mutex);
            self.ensureRegistered();
            const auto token = ++self.nextToken;
            self.hooks[token] = std::move(hook);
            return token;
        }

        static void remove(const uint64_t token) {
            auto& self = instance();
            std::lock_guard lock(self.mutex);
            self.hooks.erase(token);
        }
    };

} // ntgcalls