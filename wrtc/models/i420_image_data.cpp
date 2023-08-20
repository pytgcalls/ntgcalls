//
// Created by Laky64 on 13/08/2023.
//

#include "i420_image_data.hpp"

namespace wrtc {
    size_t i420ImageData::sizeOfLuminancePlane() const {
        return static_cast<size_t>(width * height);
    }

    size_t i420ImageData::sizeOfChromaPlane() const {
        return sizeOfLuminancePlane() / 4;
    }

    binary i420ImageData::dataY() {
        return contents;
    }

    binary i420ImageData::dataU() {
        return dataY() + sizeOfLuminancePlane();
    }

    binary i420ImageData::dataV() {
        return dataU() + sizeOfChromaPlane();
    }

    i420ImageData::i420ImageData(uint16_t width, uint16_t height, binary contents) {
        this->width = width;
        this->height = height;
        this->contents = contents;
    }

    rtc::scoped_refptr<webrtc::I420Buffer> i420ImageData::buffer() {
        auto buffer = webrtc::I420Buffer::Create(width, height);
        memcpy(buffer->MutableDataY(), dataY(), sizeOfLuminancePlane());
        memcpy(buffer->MutableDataU(), dataU(), sizeOfChromaPlane());
        memcpy(buffer->MutableDataV(), dataV(), sizeOfChromaPlane());
        return buffer;
    }
}
