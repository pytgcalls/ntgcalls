#include "SdpBuilder.hpp"

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

void SdpBuilder::addHeader(int64_t session_id) {
    add("v=0");
    add("o=- " + std::to_string(session_id) + " 2 IN IP4 0.0.0.0");
    add("s=-");
    add("t=0 0");
    add("a=group:BUNDLE 0 1");
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
    //AUDIO CODECS
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
    //END AUDIO CODECS

    //VIDEO CODECS
    add("m=video 1 RTP/SAVPF 100 101 102 103");
    add("c=IN IP4 0.0.0.0");
    add("a=mid:1");
    addTransport(transport);

    //VP8 CODEC
    add("a=rtpmap:100 VP8/90000/1");
    add("a=fmtp:100 x-google-start-bitrate=800");
    add("a=rtcp-fb:100 goog-remb");
    add("a=rtcp-fb:100 transport-cc");
    add("a=rtcp-fb:100 ccm fir");
    add("a=rtcp-fb:100 nack");
    add("a=rtcp-fb:100 nack pli");
    add("a=rtpmap:101 rtx/90000");
    add("a=fmtp:101 apt=100");

    //VP9 CODEC
    add("a=rtpmap:102 VP9/90000/1");
    add("a=rtcp-fb:102 goog-remb");
    add("a=rtcp-fb:102 transport-cc");
    add("a=rtcp-fb:102 ccm fir");
    add("a=rtcp-fb:102 nack");
    add("a=rtcp-fb:102 nack pli");
    add("a=rtpmap:103 rtx/90000");
    add("a=fmtp:103 apt=102");

    add("a=recvonly");
    add("a=rtcp:1 IN IP4 0.0.0.0");
    add("a=rtcp-mux");
    //END VIDEO CODECS
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