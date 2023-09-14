//
// Created by Laky64 on 19/08/2023.
//

#include "rtc_video_source.hpp"

namespace wrtc {
    RTCVideoSource::RTCVideoSource() {
        factory = PeerConnectionFactory::GetOrCreateDefault();
        source = new rtc::RefCountedObject<VideoTrackSource>();
    }

    RTCVideoSource::~RTCVideoSource() {
        factory = nullptr;
        source = nullptr;
        PeerConnectionFactory::UnRef();
    }

    MediaStreamTrack *RTCVideoSource::createTrack() {
        return MediaStreamTrack::holder()->GetOrCreate(
            factory->factory()->CreateVideoTrack(source, rtc::CreateRandomUuid())
        );
    }

    void RTCVideoSource::OnFrame(i420ImageData data) {
        source->PushFrame(webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(data.buffer())
            .set_timestamp_rtp(0)
            .set_timestamp_ms(rtc::TimeMillis())
            .set_timestamp_us(rtc::TimeMicros())
            .set_rotation(webrtc::kVideoRotation_0)
            .build()
        );
    }
} // wrtc