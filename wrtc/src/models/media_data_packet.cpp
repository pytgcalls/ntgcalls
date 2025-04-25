//
// Created by Laky64 on 15/04/25.
//

#include <wrtc/models/media_data_packet.hpp>

namespace wrtc {
    MediaDataPacket::MediaDataPacket(): packet(av_packet_alloc()) {}

    MediaDataPacket::MediaDataPacket(MediaDataPacket* other) : packet(other->packet) {
        other->packet = nullptr;
    }

    MediaDataPacket::~MediaDataPacket() {
        if (packet) {
            av_packet_free(&packet);
        }
    }

    AVPacket* MediaDataPacket::getPacket() const {
        return packet;
    }
} // wrtc