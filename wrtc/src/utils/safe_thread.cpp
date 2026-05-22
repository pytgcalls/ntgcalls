//
// Created by laky64 on 06/02/26.
//

#include <wrtc/utils/safe_thread.hpp>

namespace wrtc {
    std::unique_ptr<SafeThread> SafeThread::Create() {
        return std::make_unique<SafeThread>(webrtc::Thread::Create());
    }

    std::unique_ptr<SafeThread> SafeThread::CreateWithSocketServer() {
        return std::make_unique<SafeThread>(webrtc::Thread::CreateWithSocketServer());
    }

    void SafeThread::Start() const {
        thread->Start();
    }

    void SafeThread::Stop() const {
        thread->Stop();
    }

    void SafeThread::SetName(const absl::string_view name, const void* obj) const {
        thread->SetName(name, obj);
    }

    bool SafeThread::IsCurrent() const {
        return thread->IsCurrent();
    }

    void SafeThread::AllowInvokesToThread(const SafeThread& other) const {
        thread->AllowInvokesToThread(other);
    }

    void SafeThread::PostTask(absl::AnyInvocable<void() &&> task) const {
        thread->PostTask(std::move(task));
    }

    void SafeThread::PostDelayedTask(absl::AnyInvocable<void() &&> task, const webrtc::TimeDelta delay) const {
        thread->PostDelayedTask(std::move(task), delay);
    }
} // wrtc