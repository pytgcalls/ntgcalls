//
// Created by Laky64 on 02/03/2024.
//
# pragma once
#include <cstdint>

#include "dispatch_queue.hpp"
#ifdef _WIN32
#include <windows.h>
typedef DWORD pid_t;
#elif __APPLE__
#include <mach/mach.h>
#else
#include "sys/times.h"
#include <unistd.h>
#endif

namespace ntgcalls {

    class HardwareInfo {
        uint16_t numProcessors;
        std::thread infoRetriever;
#ifdef _WIN32
        ULARGE_INTEGER lastCPU{}, lastSysCPU{}, lastUserCPU{};
        HANDLE self;
#else
        clock_t lastCPU, lastSysCPU, lastUserCPU;
#endif
    public:
        HardwareInfo();

        double getCpuUsage();

        uint16_t getCoreCount() const;
    };

} // ntgcalls
