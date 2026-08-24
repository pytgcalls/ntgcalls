//
// Created by Lauren on 19/08/23.
//

#include <api/make_ref_counted.h>
#include <rtc_base/crypto_random.h>
#include <wrtc/interfaces/media/rtc_video_source.hpp>

namespace wrtc::interfaces::media {
    RTCVideoSource::RTCVideoSource() {
        factory_ = peer_connection::PeerConnectionFactory::get_or_create_default();
        source_ = webrtc::make_ref_counted<tracks::VideoTrackSource>();
    }

    RTCVideoSource::~RTCVideoSource() {
        factory_ = nullptr;
        source_ = nullptr;
    }

    webrtc::scoped_refptr<webrtc::VideoTrackInterface> RTCVideoSource::create_track() const {
        return factory_->factory()->CreateVideoTrack(source_, webrtc::CreateRandomUuid());
    }

    void RTCVideoSource::on_frame(const models::I420ImageData& data, const models::FrameData additional_data) const {
        const auto frame = webrtc::VideoFrame::Builder()
                               .set_video_frame_buffer(data.buffer())
                               .set_timestamp_rtp(0)
                               .set_timestamp_ms(additional_data.absolute_capture_timestamp_ms)
                               .set_rotation(additional_data.rotation)
                               .build();
        source_->push_frame(frame);
    }
} // wrtc::interfaces::media
