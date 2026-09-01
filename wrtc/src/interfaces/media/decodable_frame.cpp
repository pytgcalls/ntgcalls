//
// Created by Lauren on 15/04/25.
//

#include <wrtc/interfaces/media/decodable_frame.hpp>

namespace wrtc::interfaces::media {
    DecodableFrame::DecodableFrame(std::unique_ptr<MediaDataPacket> packet, const int64_t pts, const int64_t dts): packet_(std::move(packet)), pts_(pts), dts_(dts) {}

    DecodableFrame::~DecodableFrame() {
        packet_ = nullptr;
    }

    MediaDataPacket* DecodableFrame::get_packet() const {
        return packet_.get();
    }

    int64_t DecodableFrame::get_pts() const {
        return pts_;
    }

    int64_t DecodableFrame::get_dts() const {
        return dts_;
    }
} // wrtc::interfaces::media
