//
// Created by Laky64 on 15/04/25.
//

#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
}

namespace wrtc {

    class MediaDataPacket {
    public:
        MediaDataPacket();

        explicit MediaDataPacket(MediaDataPacket* other);

        ~MediaDataPacket();

        AVPacket* getPacket() const;

    private:
        AVPacket *packet = nullptr;
    };

} // wrtc
