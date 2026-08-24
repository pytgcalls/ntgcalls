//
// Created by Lauren on 15/04/25.
//

#include <wrtc/interfaces/media/media_data_packet.hpp>

namespace wrtc::interfaces::media {
    MediaDataPacket::MediaDataPacket(): packet_(av_packet_alloc()) {}

    MediaDataPacket::MediaDataPacket(MediaDataPacket* other): packet_(other->packet_) {
        other->packet_ = nullptr;
    }

    MediaDataPacket::~MediaDataPacket() {
        if (packet_) {
            av_packet_free(&packet_);
        }
    }

    AVPacket* MediaDataPacket::get_packet() const {
        return packet_;
    }
} // wrtc::interfaces::media
