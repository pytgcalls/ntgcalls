//
// Created by Lauren on 16/10/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/utils/g_lib_loop_manager.hpp>

namespace ntgcalls::utils {
    int GLibLoopManager::references_ = 0;
    bool GLibLoopManager::is_runnable_ = false;
    bool GLibLoopManager::allow_eventloop_ = true;
#ifdef IS_LINUX
    GMainLoop *GLibLoopManager::loop_ = nullptr;
    webrtc::PlatformThread GLibLoopManager::thread_;
#endif

    bool GLibLoopManager::is_event_loop_running()  {
#ifdef IS_LINUX
        GMainContext* context = g_main_context_default();
        return g_main_context_is_owner(context);
#else
        return false;
#endif
    }

    void GLibLoopManager::enable_event_loop(const bool enable) {
        if (references_ > 0 && enable != allow_eventloop_) {
            throw MediaDeviceError("Unable to re-enable event loop while instances are active");
        }
        allow_eventloop_ = enable;
    }

    void GLibLoopManager::add_instance() {
        references_++;
        if (references_ == 1) {
#ifdef IS_LINUX
            is_runnable_ = !is_event_loop_running() && allow_eventloop_;
            if (!is_runnable_) {
                return;
            }
            loop_ = g_main_loop_new(nullptr, false);
            thread_ = webrtc::PlatformThread::SpawnJoinable([] {
                g_main_loop_run(loop_);
            },"GLibLoopManager", webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime));
#endif
        }
    }

    void GLibLoopManager::remove_instance() {
        references_--;
        if (references_ == 0) {
#ifdef IS_LINUX
            if (!is_runnable_) {
                return;
            }
            g_main_loop_quit(loop_);
            g_main_loop_unref(loop_);
            loop_ = nullptr;
            thread_.Finalize();
#endif
        }
    }
} // ntgcalls::utils