//
// Created by Laky64 on 14/04/25.
//

#pragma once
#include <wrtc/utils/binary.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace wrtc {

    class AVIOContextImpl {
        bytes::binary fileData;
        int fileReadPosition = 0;
        bytes::binary buffer;
        AVIOContext *context = nullptr;

        static int AVIOContextImplRead(void *opaque, unsigned char *buffer, int bufferSize);

        static int64_t AVIOContextImplSeek(void *opaque, int64_t offset, int whence);

    public:
        explicit AVIOContextImpl(bytes::binary &&fileData);

        ~AVIOContextImpl();

        AVIOContext *getContext() const;
    };

} // wrtc
