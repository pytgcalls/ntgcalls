//
// Created by Laky64 on 16/10/24.
//

#pragma once
#ifdef IS_LINUX
#include <gio/gio.h>
#include <rtc_base/platform_thread.h>
#endif

namespace ntgcalls {

    class GLibLoopManager {
        static int references;
        static bool isRunnable;
        static bool allowEventloop;
#ifdef IS_LINUX
        static GMainLoop *loop;
        static rtc::PlatformThread thread;
#endif
        static bool isEventLoopRunning();

    public:
        static void EnableEventLoop(bool enable);

        static void AddInstance();

        static void RemoveInstance();
    };

} // ntgcalls
