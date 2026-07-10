//
// Created by Lauren on 19/08/23.
//

#include <wrtc/interfaces/media/rtc_audio_source.hpp>
#include <rtc_base/crypto_random.h>
#include <api/make_ref_counted.h>

namespace wrtc::interfaces::media {
    RTCAudioSource::RTCAudioSource() {
        factory_ = peer_connection::PeerConnectionFactory::get_or_create_default();
        source_ = webrtc::make_ref_counted<tracks::AudioTrackSource>();
    }

    RTCAudioSource::~RTCAudioSource() {
        factory_ = nullptr;
        source_ = nullptr;
    }

    webrtc::scoped_refptr<webrtc::AudioTrackInterface> RTCAudioSource::create_track() const {
        return factory_->factory()->CreateAudioTrack(webrtc::CreateRandomUuid(), source_.get());
    }

    void RTCAudioSource::on_data(const models::RTCOnDataEvent &data, const models::FrameData additional_data) const {
        source_->push_data(data, additional_data.absolute_capture_timestamp_ms);
    }
} // wrtc::interfaces::media