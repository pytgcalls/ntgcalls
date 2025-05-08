//
// Created by Laky64 on 15/04/25.
//

#pragma once
#include <memory>
#include <wrtc/models/media_data_packet.hpp>

namespace wrtc {

    class DecodableFrame {
        std::unique_ptr<MediaDataPacket> packet;
        int64_t pts = 0;
        int64_t dts = 0;

    public:
        DecodableFrame(std::unique_ptr<MediaDataPacket> packet, int64_t pts, int64_t dts);

        ~DecodableFrame();

        MediaDataPacket* getPacket() const;

        int64_t getPTS() const;

        int64_t getDTS() const;
    };

} // wrtc
