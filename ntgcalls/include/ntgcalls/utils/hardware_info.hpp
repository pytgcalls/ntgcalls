//
// Created by Laky64 on 02/03/2024.
//
# pragma once
#include <cstdint>

#ifdef IS_WINDOWS
#include <windows.h>
#elif IS_MACOS
#include <sys/sysctl.h>
#elif defined(IS_LINUX) || defined(IS_ANDROID)
#include <sys/times.h>
#endif

namespace ntgcalls {

    class HardwareInfo {
        int numProcessors;
#ifdef IS_WINDOWS
        ULARGE_INTEGER lastCPU{}, lastSysCPU{}, lastUserCPU{};
        HANDLE self;
#else
        clock_t lastCPU, lastSysCPU, lastUserCPU;
#endif
    public:
        HardwareInfo();

        double getCpuUsage();

        [[nodiscard]] uint16_t getCoreCount() const;
    };

} // ntgcalls
