//
// Created by Lauren on 13/08/23.
//

#include <wrtc/models/i420_image_data.hpp>

namespace wrtc::models {
    size_t I420ImageData::size_of_luminance_plane() const {
        return static_cast<size_t>(width_ * height_);
    }

    size_t I420ImageData::size_of_chroma_plane() const {
        return size_of_luminance_plane() / 4;
    }

    bytes::byte* I420ImageData::data_y() const {
        return contents_.get();
    }

    bytes::byte* I420ImageData::data_u() const {
        return data_y() + size_of_luminance_plane();
    }

    bytes::byte* I420ImageData::data_v() const {
        return data_u() + size_of_chroma_plane();
    }

    I420ImageData::I420ImageData(const uint16_t width, const uint16_t height, const bytes::byte* contents, const size_t size) {
        this->width_ = width;
        this->height_ = height;
        const size_t data_size = size_of_luminance_plane() + 2 * size_of_chroma_plane();
        this->contents_ = bytes::make_unique_binary(data_size);
        if (contents && size == data_size) {
            std::memcpy(this->contents_.get(), contents, data_size);
        } else {
            std::memset(this->contents_.get(), 0, data_size);
        }
    }

    I420ImageData::~I420ImageData() = default;

    webrtc::scoped_refptr<webrtc::I420Buffer> I420ImageData::buffer() const {
        auto buffer = webrtc::I420Buffer::Create(width_, height_);
        std::memcpy(buffer->MutableDataY(), data_y(), size_of_luminance_plane());
        std::memcpy(buffer->MutableDataU(), data_u(), size_of_chroma_plane());
        std::memcpy(buffer->MutableDataV(), data_v(), size_of_chroma_plane());
        return buffer;
    }
}
