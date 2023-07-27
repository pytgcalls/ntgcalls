//
// Created by iraci on 26/07/2023.
//
#include "tgcalls.hpp"
#include "rtc/rtc.hpp"


int main() {
    auto audio = rtc::Description::Audio("audio", rtc::Description::Direction::SendRecv);
    audio.addOpusCodec(103);
    audio.addOpusCodec(111);
    audio.addSSRC(generateSSRC(), "audio");

    auto video = rtc::Description::Video("video", rtc::Description::Direction::SendRecv);
    video.addH264Codec(96);
    video.addSSRC(generateSSRC(), "video");

    auto call = TgCalls();
    call.start(audio, video, [](const JoinVoiceCallParams& params) -> JoinVoiceCallResult {
        json message = {
                {"ufrag", params.ufrag},
                {"pwd", params.pwd},
                {"hash", params.hash},
                {"setup", params.setup},
                {"fingerprint", params.fingerprint},
                {"source", params.source},
                {"source_groups", params.source_groups}
        };
        std::cout << std::endl << "LocalDescription Generated: " << std::endl;
        std::cout << message << std::endl;

        std::cout << std::endl << "Please paste the remoteDescription provided by main.py: " << std::endl;
        std::string sdp;
        std::getline(std::cin, sdp);

        // NEEDED A WAY TO INTERCOMMUNICATE WITH PYTHON
        return {};
    });
    return 0;
}