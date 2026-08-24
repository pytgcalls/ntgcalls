//
// Created by Lauren on 16/10/24.
//

#pragma once
#ifdef IS_LINUX
#include <gio/gio.h>
#include <rtc_base/platform_thread.h>
#endif

namespace ntgcalls::utils {

    class GLibLoopManager {
        static int references_;
        static bool is_runnable_;
        static bool allow_eventloop_;
#ifdef IS_LINUX
        static GMainLoop* loop_;
        static webrtc::PlatformThread thread_;
#endif
        static bool is_event_loop_running();

    public:
        static void enable_event_loop(bool enable);

        static void add_instance();

        static void remove_instance();
    };

} // ntgcalls::utils
