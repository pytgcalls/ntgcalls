//
// Created by Lauren on 13/08/23.
//

#pragma once


#include <memory>
#include <api/scoped_refptr.h>
#include <api/video/i420_buffer.h>
#include <wrtc/utils/binary.hpp>

namespace wrtc::models {
    class I420ImageData {
        uint16_t width_, height_;
        bytes::unique_binary contents_;

        [[nodiscard]] size_t size_of_luminance_plane() const;

        [[nodiscard]] size_t size_of_chroma_plane() const;

        [[nodiscard]] bytes::byte* data_y() const;

        [[nodiscard]] bytes::byte* data_u() const;

        [[nodiscard]] bytes::byte* data_v() const;


    public:
        I420ImageData(uint16_t width, uint16_t height, const bytes::byte* contents, size_t size);

        ~I420ImageData();

        [[nodiscard]] webrtc::scoped_refptr<webrtc::I420Buffer> buffer() const;
    };
}
