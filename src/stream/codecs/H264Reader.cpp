//
// Created by Laky64 on 02/08/2023.
//

#include "H264Reader.hpp"

H264Reader::H264Reader(const std::string& directory, uint32_t fps): FileReader(directory, ".h264", fps, true) { }

void H264Reader::init() {
    auto video = rtc::Description::Video(desc->mid, rtc::Description::Direction::SendRecv);
    video.addH264Codec(102);
    video.addSSRC(desc->ssrc, desc->cname, desc->msid, desc->trackId);
    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(desc->ssrc, desc->cname, 102, rtc::H264RtpPacketizer::defaultClockRate);
    auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(rtc::H264RtpPacketizer::Separator::Length, rtpConfig);
    mediaHandler = std::make_shared<rtc::H264PacketizationHandler>(packetizer);
    srReporter = std::make_shared<rtc::RtcpSrReporter>(rtpConfig);
    mediaHandler->addToChain(srReporter);
    auto nackResponder = std::make_shared<rtc::RtcpNackResponder>();
    mediaHandler->addToChain(nackResponder);
    desc->attachMedia(video);
}

void H264Reader::loadNextSample() {
    FileReader::loadNextSample();

    size_t i = 0;
    while (i < sample.size()) {
        assert(i + 4 < sample.size());
        auto lengthPtr = (uint32_t *) (sample.data() + i);
        uint32_t length = ntohl(*lengthPtr);
        auto naluStartIndex = i + 4;
        auto naluEndIndex = naluStartIndex + length;
        assert(naluEndIndex <= sample.size());
        auto header = reinterpret_cast<rtc::NalUnitHeader *>(sample.data() + naluStartIndex);
        auto type = header->unitType();
        if (type == 7) {
            previousUnitType7 = {sample.begin() + i, sample.begin() + naluEndIndex};
        } else if (type == 8) {
            previousUnitType8 = {sample.begin() + i, sample.begin() + naluEndIndex};
        } else if (type == 5) {
            previousUnitType5 = {sample.begin() + i, sample.begin() + naluEndIndex};
        }
        i = naluEndIndex;
    }
}

std::vector<std::byte> H264Reader::initialNALUS() {
    std::vector<std::byte> units{};
    if (previousUnitType7.has_value()) {
        auto nalu = previousUnitType7.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    if (previousUnitType8.has_value()) {
        auto nalu = previousUnitType8.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    if (previousUnitType5.has_value()) {
        auto nalu = previousUnitType5.value();
        units.insert(units.end(), nalu.begin(), nalu.end());
    }
    return units;
}
