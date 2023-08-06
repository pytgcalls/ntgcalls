//
// Created by Laky64 on 03/08/2023.
//

#include "MediaStreamTrack.hpp"

MediaStreamTrack::MediaStreamTrack(Type codec, const std::shared_ptr<rtc::PeerConnection>& pc) {
    cname = generateUniqueId(16);
    msid = "-";
    trackId = generateTrackId();
    mid = std::to_string(codec == Audio ? 0:1);
    ssrc = generateSSRC();

    std::optional<rtc::Description::Media> desc;
    if (codec == Audio) {
        auto audio = rtc::Description::Audio(mid, rtc::Description::Direction::SendOnly);
        audio.addOpusCodec(codec);
        desc = audio;
    } else {
        auto video = rtc::Description::Video(mid, rtc::Description::Direction::SendOnly);
        video.addH264Codec(codec);
        desc = video;
    }
    desc -> addSSRC(ssrc, cname, msid, trackId);
    track = pc->addTrack(desc.value());
    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
            ssrc,
            cname,
            codec,
            codec == Audio ? rtc::OpusRtpPacketizer::defaultClockRate : rtc::H264RtpPacketizer::defaultClockRate
    );

    std::shared_ptr<rtc::MediaChainableHandler> mediaHandler;
    if (codec == Audio) {
        auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtpConfig);
        mediaHandler = std::make_shared<rtc::OpusPacketizationHandler>(packetizer);
    } else {
        auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(rtc::H264RtpPacketizer::Separator::Length, rtpConfig);
        mediaHandler = std::make_shared<rtc::H264PacketizationHandler>(packetizer);
    }
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    mediaHandler->addToChain(nackResponder);
    track->setMediaHandler(mediaHandler);
}

MediaStreamTrack::~MediaStreamTrack() {
    track = nullptr;
}

std::string MediaStreamTrack::generateTrackId() {
    const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    return generateUniqueId(8, chars) + "-" +
           generateUniqueId(4, chars) + "-" +
           generateUniqueId(4, chars) + "-" +
           generateUniqueId(12, chars);
}

std::string MediaStreamTrack::generateUniqueId(int length, const std::string& chars) {
    static std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(chars.size()) - 1);
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += chars[dist(gen)];
    }
    return result;
}

uint32_t MediaStreamTrack::generateSSRC() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, UINT32_MAX);
    return dis(gen);
}

void MediaStreamTrack::onOpen(const std::function<void()>& callback) {
    track->onOpen(callback);
}

void MediaStreamTrack::sendData(const rtc::binary& samples) {
    if (!samples.empty()) {
        std::cout << "Sending sample with size: " << std::to_string(samples.size()) << " to " << std::to_string(ssrc) << std::endl;
    }
    try {
        track->send(samples);
    } catch (const std::exception &e) {
        std::cerr << "Unable to send packet: " << e.what() << std::endl;
    }
}