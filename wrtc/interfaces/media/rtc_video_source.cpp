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
        auto now = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
        int64_t nowInUs = now.time_since_epoch().count();

        webrtc::VideoFrame::Builder builder;
        auto frame = builder
                .set_timestamp_us(nowInUs)
                .set_video_frame_buffer(data.buffer())
                .build();
        source->PushFrame(frame);
    }
} // wrtc