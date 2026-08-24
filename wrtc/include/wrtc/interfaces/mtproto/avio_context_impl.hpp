//
// Created by Lauren on 14/04/25.
//

#pragma once
#include <wrtc/utils/binary.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace wrtc::interfaces::mtproto {

    class AVIOContextImpl {
        bytes::binary file_data_;
        int file_read_position_ = 0;
        bytes::binary buffer_;
        AVIOContext* context_ = nullptr;

        static int read(void* opaque, unsigned char* buffer, int buffer_size);

        static int64_t seek(void* opaque, int64_t offset, int whence);

    public:
        explicit AVIOContextImpl(bytes::binary&& file_data);

        ~AVIOContextImpl();

        [[nodiscard]] AVIOContext* get_context() const;
    };

} // wrtc::interfaces::mtproto
