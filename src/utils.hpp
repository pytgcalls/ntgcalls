//
// Created by iraci on 26/07/2023.
//


#include <string>
#include <vector>
#include <optional>
#include "rtc/rtc.hpp"
#include <iostream>
#include <sstream>
#include <random>

struct Sdp {
    std::optional<std::string> fingerprint;
    std::optional<std::string> hash;
    std::optional<std::string> setup;
    std::optional<std::string> pwd;
    std::optional<std::string> ufrag;
    int audioSource;
    std::vector<int> source_groups;
};

struct JoinVoiceCallParams {
    std::string ufrag;
    std::string pwd;
    std::string hash;
    std::string setup;
    std::string fingerprint;
    int source;
    std::vector<int> source_groups;
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

struct JoinVoiceCallResult {
    std::optional<std::vector<Transport>> transport;
    std::string error;
};

Sdp parseSdp(const std::string& sdp);

int generateSSRC();