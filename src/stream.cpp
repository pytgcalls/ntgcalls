//
// Created by daniele on 28/07/23.
//

#include "stream.hpp"

Stream::Stream() {
    audioSource = initAudio();
}

rtc::Description::Audio Stream::initAudio() {
    auto cname = generateUniqueId(16);
    auto trackId = generateUniqueId(36);
    auto audio = rtc::Description::Audio("0");
    audio.addOpusCodec(111);
    audio.addSSRC(generateSSRC(), cname, "-", cname);
    // create RTP configuration
    auto rtpConfig = make_shared<rtc::RtpPacketizationConfig>(ssrc, cname, payloadType, rtc::OpusRtpPacketizer::defaultClockRate);
    // create packetizer
    auto packetizer = make_shared<rtc::OpusRtpPacketizer>(rtpConfig);
    // create opus handler
    auto opusHandler = make_shared<rtc::OpusPacketizationHandler>(packetizer);
    // add RTCP SR handler
    auto srReporter = make_shared<rtc::RtcpSrReporter>(rtpConfig);
    opusHandler->addToChain(srReporter);
    // add RTCP NACK handler
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    opusHandler->addToChain(nackResponder);
    return audio;
}

void Stream::addAudio() {

    // set handler
    track->setMediaHandler(opusHandler);
    track->onOpen(onOpen);
    auto trackData = make_shared<ClientTrackData>(track, srReporter);
}