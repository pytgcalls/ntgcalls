//
// Created by Lauren on 15/04/25.
//

#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
}

namespace wrtc::interfaces::media {

    class MediaDataPacket {
        AVPacket* packet_ = nullptr;

    public:
        MediaDataPacket();

        explicit MediaDataPacket(MediaDataPacket* other);

        ~MediaDataPacket();

        [[nodiscard]] AVPacket* get_packet() const;
    };

} // wrtc::interfaces::media
