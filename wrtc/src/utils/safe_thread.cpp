//
// Created by Lauren on 06/02/26.
//

#include <wrtc/utils/safe_thread.hpp>

namespace wrtc::utils {
    std::unique_ptr<SafeThread> SafeThread::Create() {
        return std::make_unique<SafeThread>(webrtc::Thread::Create());
    }

    std::unique_ptr<SafeThread> SafeThread::CreateWithSocketServer() {
        return std::make_unique<SafeThread>(webrtc::Thread::CreateWithSocketServer());
    }

    void SafeThread::Start() const {
        thread_->Start();
    }

    void SafeThread::Stop() const {
        thread_->Stop();
    }

    void SafeThread::SetName(const absl::string_view name, const void* obj) const {
        thread_->SetName(name, obj);
    }

    bool SafeThread::IsCurrent() const {
        return thread_->IsCurrent();
    }

    webrtc::SocketServer* SafeThread::socketServer() const {
        return thread_->socketserver();
    }

    void SafeThread::AllowInvokesToThread(const SafeThread& other) const {
        thread_->AllowInvokesToThread(other);
    }

    void SafeThread::PostTask(absl::AnyInvocable<void() &&> task) const {
        thread_->PostTask(std::move(task));
    }

    void SafeThread::PostDelayedTask(absl::AnyInvocable<void() &&> task, const webrtc::TimeDelta delay) const {
        thread_->PostDelayedTask(std::move(task), delay);
    }
} // wrtc::utils