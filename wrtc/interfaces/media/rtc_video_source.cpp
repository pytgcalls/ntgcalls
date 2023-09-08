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
        webrtc::VideoFrame::Builder builder;
        builder.set_timestamp_us(rtc::TimeMicros());
        builder.set_video_frame_buffer(data.buffer());
        source->PushFrame(builder.build());
    }
} // wrtc