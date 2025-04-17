//
// Created by Laky64 on 15/04/25.
//

#include <wrtc/models/decodable_frame.hpp>

namespace wrtc {
    DecodableFrame::DecodableFrame(std::unique_ptr<MediaDataPacket> packet, const int64_t pts, const int64_t dts): packet(std::move(packet)), pts(pts), dts(dts){}

    DecodableFrame::~DecodableFrame() {
        packet = nullptr;
    }

    MediaDataPacket* DecodableFrame::getPacket() const {
        return packet.get();
    }

    int64_t DecodableFrame::getPTS() const {
        return pts;
    }

    int64_t DecodableFrame::getDTS() const {
        return dts;
    }
} // wrtc