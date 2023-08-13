//
// Created by Laky64 on 12/08/2023.
//

#include "rtc_video_source.hpp"

namespace wrtc {

    RTCVideoSource::RTCVideoSource() {
        _source = new rtc::RefCountedObject<RTCVideoTrackSource>();
    }

    RTCVideoSource::RTCVideoSource(bool is_screencast, absl::optional<bool> needs_denoising) {
        _source = new rtc::RefCountedObject<RTCVideoTrackSource>(is_screencast, needs_denoising);
    }

    MediaStreamTrack *RTCVideoSource::createTrack() {
        // TODO(mroberts): Again, we have some implicit factory we are threading around. How to handle?
        auto factory = PeerConnectionFactory::GetOrCreateDefault();
        auto track = factory->factory()->CreateVideoTrack(rtc::CreateRandomUuid(), _source.get());
        return MediaStreamTrack::holder()->GetOrCreate(factory, track);
    }

    void RTCVideoSource::OnFrame(i420ImageData data) {
        auto now = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
        int64_t nowInUs = now.time_since_epoch().count();

        webrtc::VideoFrame::Builder builder;
        auto frame = builder
                .set_timestamp_us(nowInUs)
                .set_video_frame_buffer(data.buffer())
                .build();
        _source->PushFrame(frame);
    }
}
