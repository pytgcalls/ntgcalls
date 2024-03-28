//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <vector>
#include <string>

#include "enums.hpp"

namespace wrtc {

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

    struct Conference {
        Transport transport;
        SSRC ssrc;
        std::vector<SSRC> source_groups;
    };

    struct Sdp {
        std::string fingerprint;
        std::string hash;
        std::string setup;
        std::string pwd;
        std::string ufrag;
        SSRC audioSource;
        std::vector<SSRC> source_groups;
    };

    class SdpBuilder {
        std::vector<std::string> lines;
        std::vector<std::string> newLine;

        void add(const std::string& line);
        void push(const std::string& word);
        void addJoined(const std::string& separator = "");
        void addCandidate(const Candidate& c);
        void addHeader();
        void addTransport(const Transport& transport);
        void addSsrcEntry(const Transport& transport);

        [[nodiscard]] std::string join() const;
        [[nodiscard]] std::string finalize() const;
        void addConference(const Conference& conference);

    public:
        static std::string fromConference(const Conference& conference);

        static Sdp parseSdp(const std::string& sdp);
    };
}