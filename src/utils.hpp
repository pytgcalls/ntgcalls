//
// Created by Laky64 on 26/07/23.
//

#ifndef NTGCALLS_UTILS_HPP
#define NTGCALLS_UTILS_HPP

#include <string>
#include <vector>
#include <optional>
#include "rtc/rtc.hpp"
#include <iostream>
#include <sstream>
#include <random>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

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

struct Sdp {
    std::optional<std::string> fingerprint;
    std::optional<std::string> hash;
    std::optional<std::string> setup;
    std::optional<std::string> pwd;
    std::optional<std::string> ufrag;
    uint32_t audioSource;
    std::vector<uint32_t> source_groups;
};

struct JoinVoiceCallParams {
    std::string ufrag;
    std::string pwd;
    std::string hash;
    std::string setup;
    std::string fingerprint;
};

struct Fingerprint {
    std::string hash;
    std::string fingerprint;
};

struct Candidate {
    std::string generation;
    std::string component;
    std::string protocol;
    std::string port;
    std::string ip;
    std::string foundation;
    std::string id;
    std::string priority;
    std::string type;
    std::string network;
};

struct Transport {
    std::string ufrag;
    std::string pwd;
    std::vector<Fingerprint> fingerprints;
    std::vector<Candidate> candidates;
};

struct Ssrc {
    uint32_t ssrc;
    std::vector<uint32_t> ssrc_group;
};

struct Conference {
    int64_t session_id;
    Transport transport;
    std::vector<Ssrc> ssrcs;
};

Sdp parseSdp(const std::string& sdp);

uint32_t generateSSRC();

int64_t getMilliseconds();

uint64_t getMicroseconds();

std::string generateUniqueId(int length);

inline uint32_t ntohl(uint32_t networkInt) {
    return ((networkInt & 0xFF000000u) >> 24) |
           ((networkInt & 0x00FF0000u) >> 8)  |
           ((networkInt & 0x0000FF00u) << 8)  |
           ((networkInt & 0x000000FFu) << 24);
}

#endif