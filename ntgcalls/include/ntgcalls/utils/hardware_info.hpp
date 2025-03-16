//
// Created by Laky64 on 02/03/2024.
//
# pragma once
#include <cstdint>

#include <thread>
#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <sys/sysctl.h>
#else
#include <sys/times.h>
#include <unistd.h>
#endif

namespace ntgcalls {

    class HardwareInfo {
        uint16_t numProcessors;
#ifdef _WIN32
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
