//
// Created by Laky64 on 03/08/2023.
//

#ifndef NTGCALLS_TIME_HPP
#define NTGCALLS_TIME_HPP

#include <cstdint>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
inline void usleep(__int64 uSec) {
    HANDLE timer;
    LARGE_INTEGER ft;
    ft.QuadPart = -(10 * uSec);
    timer = CreateWaitableTimer(nullptr, TRUE, nullptr);
    SetWaitableTimer(timer, &ft, 0, nullptr, nullptr, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#else
#include <unistd.h>
#endif

int64_t getMilliseconds();

uint64_t getMicroseconds();

#endif //NTGCALLS_TIME_HPP
