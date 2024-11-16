//
// Created by Laky64 on 19/08/2023.
//

#include <wrtc/interfaces/media/rtc_video_source.hpp>
#include <rtc_base/crypto_random.h>

namespace wrtc {
    RTCVideoSource::RTCVideoSource() {
        factory = PeerConnectionFactory::GetOrCreateDefault();
        source = new rtc::RefCountedObject<VideoTrackSource>();
    }

    RTCVideoSource::~RTCVideoSource() {
        factory = nullptr;
        source = nullptr;
    }

    rtc::scoped_refptr<webrtc::VideoTrackInterface> RTCVideoSource::createTrack() const {
        return factory->factory()->CreateVideoTrack(source, rtc::CreateRandomUuid());
    }

    void RTCVideoSource::OnFrame(const i420ImageData& data, const FrameData additionalData) const {
        source->PushFrame(webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(data.buffer())
            .set_timestamp_rtp(0)
            .set_timestamp_ms(additionalData.absoluteCaptureTimestampMs)
            .set_rotation(additionalData.rotation)
            .build()
        );
    }
} // wrtc