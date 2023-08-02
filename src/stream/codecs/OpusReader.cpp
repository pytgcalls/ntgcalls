//
// Created by Laky64 on 01/08/2023.
//

#include "OpusReader.hpp"

OpusReader::OpusReader(const std::string& directory, uint32_t samplesPerSecond): FileReader(directory, ".opus", samplesPerSecond, false) { }

void OpusReader::init() {
    auto audio = rtc::Description::Audio(desc->mid, rtc::Description::Direction::SendRecv);
    audio.addOpusCodec(111);
    audio.addSSRC(desc->ssrc, desc->cname, desc->msid, desc->trackId);
    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(desc->ssrc, desc->cname, 111, rtc::OpusRtpPacketizer::defaultClockRate);
    auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtpConfig);
    mediaHandler = std::make_shared<rtc::OpusPacketizationHandler>(packetizer);
    srReporter = std::make_shared<rtc::RtcpSrReporter>(rtpConfig);
    mediaHandler->addToChain(srReporter);
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    mediaHandler->addToChain(nackResponder);
    desc->attachMedia(audio);
    std::cout << "Attached media audio";
}