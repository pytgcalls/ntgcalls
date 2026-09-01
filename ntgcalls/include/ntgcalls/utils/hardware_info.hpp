//
// Created by Lauren on 02/03/24.
//
#pragma once
#include <cstdint>

#ifdef IS_WINDOWS
#include <windows.h>
#elif IS_MACOS
#include <sys/sysctl.h>
#elif defined(IS_LINUX) || defined(IS_ANDROID)
#include <sys/times.h>
#endif

namespace ntgcalls::utils {

    class HardwareInfo {
        int num_processors_;
#ifdef IS_WINDOWS
        ULARGE_INTEGER last_cpu_{}, last_sys_cpu_{}, last_user_cpu_{};
        HANDLE self;
#else
        clock_t last_cpu_, last_sys_cpu_, last_user_cpu_;
#endif
    public:
        HardwareInfo();

        double get_cpu_usage();

        [[nodiscard]] uint16_t get_core_count() const;
    };

} // ntgcalls::utils
