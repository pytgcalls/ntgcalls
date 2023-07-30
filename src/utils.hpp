//
// Created by Laky64 on 26/07/23.
//

#ifndef NTGCALLS_UTILS_H
#define NTGCALLS_UTILS_H

#include <string>
#include <vector>
#include <optional>
#include "rtc/rtc.hpp"
#include <iostream>
#include <sstream>
#include <random>
#include <chrono>

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
    std::int64_t network;
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

struct MediaHandler {
    rtc::Description::Media mediaDescription;
    std::shared_ptr<rtc::MediaHandler> handler;
    std::shared_ptr<rtc::RtcpSrReporter> srReporter;
    std::shared_ptr<rtc::Track> track;
};

Sdp parseSdp(const std::string& sdp);

uint32_t generateSSRC();

int64_t getMilliseconds();

std::string generateUniqueId(int length);

#endif