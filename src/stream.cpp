#include "stream.hpp"

Stream Stream::Audio() {
    Stream stream;
    stream.source = initAudio();
    return stream;
}

Stream Stream::Video() {
    Stream stream;
    stream.source = initVideo();
    return stream;
}

std::shared_ptr<MediaHandler> Stream::initAudio() {
    auto desc = MediaDescription(0);
    auto audio = rtc::Description::Audio(desc.mid);
    audio.addOpusCodec(111);
    audio.addSSRC(desc.ssrc, desc.cname, desc.msid, desc.trackId);
    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(desc.ssrc, desc.cname, 111, rtc::OpusRtpPacketizer::defaultClockRate);
    auto packetizer = std::make_shared<rtc::OpusRtpPacketizer>(rtpConfig);
    auto opusHandler = std::make_shared<rtc::OpusPacketizationHandler>(packetizer);
    auto srReporter = std::make_shared<rtc::RtcpSrReporter>(rtpConfig);
    opusHandler->addToChain(srReporter);
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    opusHandler->addToChain(nackResponder);
    return std::make_shared<MediaHandler>(MediaHandler{
        audio,
        opusHandler,
        srReporter
    });
}

std::shared_ptr<MediaHandler> Stream::initVideo() {
    auto desc = MediaDescription(1);
    auto video = rtc::Description::Video(desc.mid);
    video.addH264Codec(102);
    video.addSSRC(desc.ssrc, desc.cname, desc.msid, desc.trackId);
    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(desc.ssrc, desc.cname, 102, rtc::H264RtpPacketizer::defaultClockRate);
    auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(rtc::H264RtpPacketizer::Separator::Length, rtpConfig);
    auto h264Handler = std::make_shared<rtc::H264PacketizationHandler>(packetizer);
    auto srReporter = std::make_shared<rtc::RtcpSrReporter>(rtpConfig);
    h264Handler->addToChain(srReporter);
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    h264Handler->addToChain(nackResponder);
    return std::make_shared<MediaHandler>(MediaHandler{
        video,
        h264Handler,
        srReporter
    });
}

std::shared_ptr<rtc::Track> Stream::addTrack(const std::shared_ptr<rtc::PeerConnection>& pc) const {
    source->track = pc->addTrack(source->mediaDescription);
    source->track->setMediaHandler(source->handler);
    source->track->onOpen([this]() {startStreaming();});
    return source->track;
}

void Stream::startStreaming() const {

}
