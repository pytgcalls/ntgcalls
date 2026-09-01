//
// Created by Lauren on 15/04/25.
//

#include <wrtc/interfaces/mtproto/extract_cv_pixel_buffer.hpp>
#include <sdk/objc/native/src/objc_frame_buffer.h>
#import <sdk/objc/components/video_frame_buffer/RTCCVPixelBuffer.h>

namespace wrtc::interfaces::mtproto {

    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> extract_cv_pixel_buffer(void* data) {
        auto pixel_buffer = (CVPixelBufferRef) (void*) data;
        return webrtc::make_ref_counted<webrtc::ObjCFrameBuffer>([[RTCCVPixelBuffer alloc] initWithPixelBuffer:pixel_buffer]);
    }

} // wrtc::interfaces::mtproto
