//
// Created by Laky64 on 15/04/25.
//

#include <wrtc/interfaces/mtproto/extract_cv_pixel_buffer.hpp>
#include <sdk/objc/native/src/objc_frame_buffer.h>
#import <sdk/objc/components/video_frame_buffer/RTCCVPixelBuffer.h>

namespace wrtc {

    rtc::scoped_refptr<webrtc::VideoFrameBuffer> extractCVPixelBuffer(void *data) {
        auto pixelBuffer = (CVPixelBufferRef)(void *)data;
        return rtc::make_ref_counted<webrtc::ObjCFrameBuffer>([[RTCCVPixelBuffer alloc] initWithPixelBuffer:pixelBuffer]);
    }

} // wrtc