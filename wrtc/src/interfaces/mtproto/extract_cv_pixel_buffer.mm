//
// Created by Laky64 on 15/04/25.
//

#include <sdk/objc/native/src/objc_frame_buffer.h>
#import <sdk/objc/components/video_frame_buffer/RTCCVPixelBuffer.h>
#include <wrtc/interfaces/mtproto/extract_cv_pixel_buffer.hpp>

namespace wrtc {

    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> extractCVPixelBuffer(const AVFrame *frame) {
        if (!frame) {
            return nullptr;
        }
        if (frame->format == AV_PIX_FMT_VIDEOTOOLBOX && frame->data[3]) {
            auto pixelBuffer = (CVPixelBufferRef)(void *)frame->data[3];
            return rtc::make_ref_counted<webrtc::ObjCFrameBuffer>([[RTCCVPixelBuffer alloc] initWithPixelBuffer:pixelBuffer]);
        }
        return nullptr;
    }

} // wrtc