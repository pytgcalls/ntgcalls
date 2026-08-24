//
// Created by Lauren on 06/02/26.
//

#pragma once
#include <type_traits>
#include <rtc_base/thread.h>

namespace wrtc::utils {
    class SafeThread {
        std::unique_ptr<webrtc::Thread> thread_;

    public:
        explicit SafeThread(std::unique_ptr<webrtc::Thread> t): thread_(std::move(t)) {}

        static std::unique_ptr<SafeThread> Create(); // NOLINT(*-identifier-naming)

        static std::unique_ptr<SafeThread> CreateWithSocketServer(); // NOLINT(*-identifier-naming)

        void Start() const; // NOLINT(*-identifier-naming)

        void Stop() const; // NOLINT(*-identifier-naming)

        operator webrtc::Thread*() const { // NOLINT
            return thread_.get();
        }

        void SetName(absl::string_view name, const void* obj) const; // NOLINT(*-identifier-naming)

        [[nodiscard]] bool IsCurrent() const; // NOLINT(*-identifier-naming)

        [[nodiscard]] webrtc::SocketServer* socketServer() const; // NOLINT(*-identifier-naming)

        void AllowInvokesToThread(const SafeThread& other) const; // NOLINT(*-identifier-naming)

        template<
            typename Functor,
            typename = std::enable_if_t<std::is_void_v<std::invoke_result_t<Functor>>>>
        void BlockingCall(Functor&& functor) { // NOLINT(*-identifier-naming)
            if (thread_->IsCurrent()) {
                functor();
            } else {
                thread_->BlockingCall(std::forward<Functor>(functor));
            }
        }

        template<
            typename Functor,
            typename R = std::invoke_result_t<Functor>,
            typename = std::enable_if_t<!std::is_void_v<R>>>
        R BlockingCall(Functor&& functor) { // NOLINT(*-identifier-naming)
            if (thread_->IsCurrent()) {
                return functor();
            }
            return thread_->BlockingCall(std::forward<Functor>(functor));
        }

        void PostTask(absl::AnyInvocable<void() &&> task) const; // NOLINT(*-identifier-naming)

        void PostDelayedTask(absl::AnyInvocable<void() &&> task, webrtc::TimeDelta delay) const; // NOLINT(*-identifier-naming)
    };
} // wrtc::utils
