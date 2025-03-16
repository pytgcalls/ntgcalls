//
// Created by Laky64 on 16/10/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/utils/g_lib_loop_manager.hpp>

namespace ntgcalls {
    int GLibLoopManager::references = 0;
    bool GLibLoopManager::isRunnable = false;
    bool GLibLoopManager::allowEventloop = true;
#ifdef IS_LINUX
    GMainLoop *GLibLoopManager::loop = nullptr;
    rtc::PlatformThread GLibLoopManager::thread;
#endif

    bool GLibLoopManager::isEventLoopRunning()  {
#ifdef IS_LINUX
        GMainContext* context = g_main_context_default();
        return g_main_context_is_owner(context);
#else
        return false;
#endif
    }

    void GLibLoopManager::EnableEventLoop(const bool enable) {
        if (references > 0 && enable != allowEventloop) {
            throw MediaDeviceError("Unable to re-enable event loop while instances are active");
        }
        allowEventloop = enable;
    }

    void GLibLoopManager::AddInstance() {
        references++;
        if (references == 1) {
#ifdef IS_LINUX
            isRunnable = !isEventLoopRunning() && allowEventloop;
            if (!isRunnable) {
                return;
            }
            loop = g_main_loop_new(nullptr, false);
            thread = rtc::PlatformThread::SpawnJoinable([] {
                g_main_loop_run(loop);
            },"GLibLoopManager", rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime));
#endif
        }
    }

    void GLibLoopManager::RemoveInstance() {
        references--;
        if (references == 0) {
#ifdef IS_LINUX
            if (!isRunnable) {
                return;
            }
            g_main_loop_quit(loop);
            g_main_loop_unref(loop);
            loop = nullptr;
            thread.Finalize();
#endif
        }
    }
} // ntgcalls