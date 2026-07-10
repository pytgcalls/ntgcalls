//
// Created by Lauren on 15/04/25.
//

#pragma once
#include <memory>
#include <wrtc/interfaces/media/media_data_packet.hpp>

namespace wrtc::interfaces::media {

    class DecodableFrame {
        std::unique_ptr<MediaDataPacket> packet_;
        int64_t pts_ = 0;
        int64_t dts_ = 0;

    public:
        DecodableFrame(std::unique_ptr<MediaDataPacket> packet, int64_t pts, int64_t dts);

        ~DecodableFrame();

        [[nodiscard]] MediaDataPacket* get_packet() const;

        [[nodiscard]] int64_t get_pts() const;

        [[nodiscard]] int64_t get_dts() const;
    };

} // wrtc::interfaces::media
