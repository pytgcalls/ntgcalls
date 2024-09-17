//
// Created by Laky64 on 13/08/2023.
//

#include <wrtc/models/i420_image_data.hpp>

namespace wrtc {
    size_t i420ImageData::sizeOfLuminancePlane() const {
        return static_cast<size_t>(width * height);
    }

    size_t i420ImageData::sizeOfChromaPlane() const {
        return sizeOfLuminancePlane() / 4;
    }

    uint8_t* i420ImageData::dataY() const {
        return contents;
    }

    uint8_t* i420ImageData::dataU() const {
        return dataY() + sizeOfLuminancePlane();
    }

    uint8_t* i420ImageData::dataV() const {
        return dataU() + sizeOfChromaPlane();
    }

    i420ImageData::i420ImageData(const uint16_t width, const uint16_t height, uint8_t* contents) {
        this->width = width;
        this->height = height;
        this->contents = contents;
    }

    i420ImageData::~i420ImageData() {
        contents = nullptr;
    }

    rtc::scoped_refptr<webrtc::I420Buffer> i420ImageData::buffer() const {
        auto buffer = webrtc::I420Buffer::Create(width, height);
        memcpy(buffer->MutableDataY(), dataY(), sizeOfLuminancePlane());
        memcpy(buffer->MutableDataU(), dataU(), sizeOfChromaPlane());
        memcpy(buffer->MutableDataV(), dataV(), sizeOfChromaPlane());
        return buffer;
    }
}
