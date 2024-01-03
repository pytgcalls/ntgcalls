//
// Created by Laky64 on 13/08/2023.
//

#pragma once


#include <api/scoped_refptr.h>
#include <api/video/i420_buffer.h>

#include "rtc_on_data_event.hpp"

namespace wrtc {
    class i420ImageData {
        uint16_t width, height;
        binary contents;

        [[nodiscard]] size_t sizeOfLuminancePlane() const;

        [[nodiscard]] size_t sizeOfChromaPlane() const;

        [[nodiscard]] uint8_t* dataY() const;

        [[nodiscard]] uint8_t* dataU() const;

        [[nodiscard]] uint8_t* dataV() const;


    public:
        i420ImageData(uint16_t width, uint16_t height, const binary& contents);

        ~i420ImageData();

        [[nodiscard]] rtc::scoped_refptr<webrtc::I420Buffer> buffer() const;
    };
}
