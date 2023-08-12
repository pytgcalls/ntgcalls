//
// Created by Laky64 on 12/08/2023.
//

#include "sdp_builder.hpp"

namespace wrtc {
    std::string SdpBuilder::join() {
        std::string joinedSdp;
        for (const auto& line : lines) {
            joinedSdp += line + "\n";
        }
        return joinedSdp;
    }

    std::string SdpBuilder::finalize() {
        return join() + "\n";
    }

    void SdpBuilder::add(const std::string& line) {
        lines.push_back(line);
    }

    void SdpBuilder::push(const std::string& word) {
        newLine.push_back(word);
    }

    void SdpBuilder::addJoined(const std::string& separator) {
        std::string joinedLine;
        for (size_t i = 0; i < newLine.size(); ++i) {
            joinedLine += newLine[i];
            if (i != newLine.size() - 1) {
                joinedLine += separator;
            }
        }
        add(joinedLine);
        newLine.clear();
    }

    void SdpBuilder::addCandidate(const Candidate& c) {
        push("a=candidate:");
        push(c.foundation + " " + c.component + " " + c.protocol + " " + c.priority + " " +
             c.ip + " " + c.port + " typ " + c.type);
        push(" generation " + c.generation);
        addJoined();
    }

    void SdpBuilder::addHeader(SessionID session_id) {
        add("v=0");
        add("o=- " + std::to_string(session_id) + " 2 IN IP4 0.0.0.0");
        add("s=-");
        add("t=0 0");
        add("a=group:BUNDLE 0");
        add("a=ice-lite");
    }

    void SdpBuilder::addTransport(const Transport& transport) {
        add("a=ice-ufrag:" + transport.ufrag);
        add("a=ice-pwd:" + transport.pwd);

        for (const auto& fingerprint : transport.fingerprints) {
            add("a=fingerprint:" + fingerprint.hash + " " + fingerprint.fingerprint);
            add("a=setup:passive");
        }

        for (const auto& candidate : transport.candidates) {
            addCandidate(candidate);
        }
    }

    void SdpBuilder::addSsrcEntry(const Transport& transport) {
        add("m=audio 1 RTP/SAVPF 111 126");
        add("c=IN IP4 0.0.0.0");
        add("a=mid:0");

        addTransport(transport);

        add("a=rtpmap:111 opus/48000/2");
        add("a=rtpmap:126 telephone-event/8000");
        add("a=fmtp:111 minptime=10; useinbandfec=1; usedtx=1");
        add("a=rtcp:1 IN IP4 0.0.0.0");
        add("a=rtcp-mux");
        add("a=rtcp-fb:111 transport-cc");
        add("a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level");
        add("a=recvonly");
    }

    void SdpBuilder::addConference(const Conference& conference) {
        addHeader(conference.session_id);
        addSsrcEntry(conference.transport);
    }

    std::string SdpBuilder::fromConference(const Conference& conference) {
        SdpBuilder sdp;
        sdp.addConference(conference);
        return sdp.finalize();
    }

    Sdp SdpBuilder::parseSdp(const std::string& sdp) {
        std::vector<std::string> lines;
        std::string line;
        std::istringstream stream(sdp);
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.push_back(line);
        }

        auto lookup = [&lines](const std::string& prefix) -> std::string {
            for (const auto& line : lines) {
                if (line.compare(0, prefix.size(), prefix) == 0) {
                    return line.substr(prefix.size());
                }
            }
            return "";
        };

        std::string rawAudioSource = lookup("a=ssrc:");
        std::string rawVideoSource = lookup("a=ssrc-group:FID ");
        return {
                lookup("a=fingerprint:").substr(lookup("a=fingerprint:").find(' ') + 1),
                lookup("a=fingerprint:").substr(0, lookup("a=fingerprint:").find(' ')),
                lookup("a=setup:"),
                lookup("a=ice-pwd:"),
                lookup("a=ice-ufrag:"),
                static_cast<SSRC>(rawAudioSource.empty() ? 0 : std::stoul(rawAudioSource.substr(0, rawAudioSource.find(' ')))),
                rawVideoSource.empty() ? std::vector<SSRC>() : std::vector<SSRC>{static_cast<SSRC>(std::stoul(rawVideoSource.substr(0, rawVideoSource.find(' '))))}
        };
    }
}