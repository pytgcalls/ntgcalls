//
// Created by Laky64 on 06/02/26.
//

#pragma once
#include <type_traits>
#include <rtc_base/thread.h>

namespace wrtc {
    class SafeThread {
        std::unique_ptr<webrtc::Thread> thread;

    public:
        explicit SafeThread(std::unique_ptr<webrtc::Thread> t) : thread(std::move(t)) {}

        static std::unique_ptr<SafeThread> Create();

        static std::unique_ptr<SafeThread> CreateWithSocketServer();

        void Start() const;

        void Stop() const;

        operator webrtc::Thread*() const { // NOLINT
            return thread.get();
        }

        void SetName(absl::string_view name, const void* obj) const;

        [[nodiscard]] bool IsCurrent() const;

        void AllowInvokesToThread(const SafeThread& other) const;

        template <
            typename Functor,
            typename = std::enable_if_t<std::is_void_v<std::invoke_result_t<Functor>>>
        >
        void BlockingCall(Functor&& functor) {
            if (thread->IsCurrent()) {
                functor();
            } else {
                thread->BlockingCall(std::forward<Functor>(functor));
            }
        }

        template <
            typename Functor,
            typename R = std::invoke_result_t<Functor>,
            typename = std::enable_if_t<!std::is_void_v<R>>
        >
        R BlockingCall(Functor&& functor) {
            if (thread->IsCurrent()) {
                return functor();
            }
            return thread->BlockingCall(std::forward<Functor>(functor));
        }

        void PostTask(absl::AnyInvocable<void() &&> task) const;

        void PostDelayedTask(absl::AnyInvocable<void()&&> task, webrtc::TimeDelta delay) const;
    };
} // wrtc
