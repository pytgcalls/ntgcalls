//
// Created by Lauren on 02/03/24.
//

#include <ntgcalls/utils/hardware_info.hpp>

#if defined(IS_LINUX) || defined(IS_ANDROID)
#include <unistd.h>
#elif IS_MACOS
#include <sys/resource.h>
#elif IS_WINDOWS
#include <cstring>
#endif

namespace ntgcalls::utils {
    HardwareInfo::HardwareInfo() {
#ifdef IS_WINDOWS
        FILETIME ftime, fsys, fuser;
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        num_processors_ = static_cast<int>(sysInfo.dwNumberOfProcessors);
        GetSystemTimeAsFileTime(&ftime);
        std::memcpy(&last_cpu_, &ftime, sizeof(FILETIME));

        self = GetCurrentProcess();
        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        std::memcpy(&last_sys_cpu_, &fsys, sizeof(FILETIME));
        std::memcpy(&last_user_cpu_, &fuser, sizeof(FILETIME));
#elif IS_MACOS
        size_t len = sizeof(num_processors_);
        sysctlbyname("hw.ncpu", &num_processors_, &len, NULL, 0);
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        last_cpu_ = usage.ru_utime.tv_sec * 1000000 + usage.ru_utime.tv_usec + usage.ru_stime.tv_sec * 1000000 + usage.ru_stime.tv_usec;
        last_sys_cpu_ = usage.ru_stime.tv_sec * 1000000 + usage.ru_stime.tv_usec;
        last_user_cpu_ = usage.ru_utime.tv_sec * 1000000 + usage.ru_utime.tv_usec;
#else
        num_processors_ = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
        tms time_sample{};
        last_cpu_ = times(&time_sample);
        last_sys_cpu_ = time_sample.tms_stime;
        last_user_cpu_ = time_sample.tms_utime;
#endif
    }

    double HardwareInfo::get_cpu_usage() {
        double percent;
#ifdef IS_WINDOWS
        FILETIME ftime, fsys, fuser;
        ULARGE_INTEGER now, sys, user;

        GetSystemTimeAsFileTime(&ftime);
        std::memcpy(&now, &ftime, sizeof(FILETIME));

        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        std::memcpy(&sys, &fsys, sizeof(FILETIME));
        std::memcpy(&user, &fuser, sizeof(FILETIME));
        percent = static_cast<double>(sys.QuadPart - last_sys_cpu_.QuadPart + user.QuadPart - last_user_cpu_.QuadPart);
        percent /= static_cast<double>(now.QuadPart - last_cpu_.QuadPart);
        percent /= num_processors_;
        last_cpu_ = now;
        last_user_cpu_ = user;
        last_sys_cpu_ = sys;
#elif IS_MACOS
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        const clock_t now = (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000000 + (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
        if (now <= last_cpu_ || usage.ru_stime.tv_sec < last_sys_cpu_ || usage.ru_utime.tv_sec < last_user_cpu_) {
            percent = -1.0;
        } else {
            percent = static_cast<double>(usage.ru_stime.tv_sec - last_sys_cpu_ + usage.ru_utime.tv_sec - last_user_cpu_);
            percent /= static_cast<double>(now - last_cpu_);
            percent /= num_processors_;
            percent *= 100;
        }
        last_cpu_ = now;
        last_sys_cpu_ = usage.ru_stime.tv_sec;
        last_user_cpu_ = usage.ru_utime.tv_sec;
#else
        tms time_sample{};
        const auto now = times(&time_sample);
        if (now <= last_cpu_ || time_sample.tms_stime < last_sys_cpu_ || time_sample.tms_utime < last_user_cpu_) {
            percent = -1.0;
        } else {
            percent = static_cast<double>(time_sample.tms_stime - last_sys_cpu_ + time_sample.tms_utime - last_user_cpu_);
            percent /= static_cast<double>(now - last_cpu_);
            percent /= num_processors_;
            percent *= 100;
        }
        last_cpu_ = now;
        last_sys_cpu_ = time_sample.tms_stime;
        last_user_cpu_ = time_sample.tms_utime;
#endif
        return percent;
    }

    uint16_t HardwareInfo::get_core_count() const {
        return num_processors_;
    }
} // ntgcalls::utils
