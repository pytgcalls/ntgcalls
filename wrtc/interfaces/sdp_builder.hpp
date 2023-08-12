//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <vector>
#include <string>
#include <optional>
#include <sstream>

namespace wrtc {
    typedef uint32_t SSRC;
    typedef int64_t SessionID;

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
        SessionID session_id;
        Transport transport;
        SSRC ssrc;
        std::vector<SSRC> source_groups;
    };

    struct Sdp {
        std::optional<std::string> fingerprint;
        std::optional<std::string> hash;
        std::optional<std::string> setup;
        std::optional<std::string> pwd;
        std::optional<std::string> ufrag;
        SSRC audioSource;
        std::vector<SSRC> source_groups;
    };

    class SdpBuilder {
    private:
        std::vector<std::string> lines;
        std::vector<std::string> newLine;

        void add(const std::string& line);
        void push(const std::string& word);
        void addJoined(const std::string& separator = "");
        void addCandidate(const Candidate& c);
        void addHeader(SessionID session_id);
        void addTransport(const Transport& transport);
        void addSsrcEntry(const Transport& transport);

        std::string join();
        std::string finalize();
        void addConference(const Conference& conference);

    public:
        static std::string fromConference(const Conference& conference);
        static Sdp parseSdp(const std::string& sdp);
    };
}