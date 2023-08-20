//
// Created by Laky64 on 13/08/2023.
//

#pragma once


#include <api/scoped_refptr.h>
#include <api/video/i420_buffer.h>

#include "rtc_on_data_event.hpp"

namespace wrtc {
    class i420ImageData {
    private:
        uint16_t width, height;
        binary contents;

        size_t sizeOfLuminancePlane() const;

        size_t sizeOfChromaPlane() const;

        binary dataY();

        uint8_t* dataU();

        uint8_t* dataV();


    public:
        i420ImageData(uint16_t width, uint16_t height, binary contents);

        rtc::scoped_refptr<webrtc::I420Buffer> buffer();
    };
}
