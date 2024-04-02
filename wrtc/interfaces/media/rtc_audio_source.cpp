//
// Created by Laky64 on 19/08/2023.
//

#include "rtc_audio_source.hpp"


namespace wrtc {
    RTCAudioSource::RTCAudioSource() {
        factory = PeerConnectionFactory::GetOrCreateDefault();
        source = new rtc::RefCountedObject<AudioTrackSource>();
    }

    RTCAudioSource::~RTCAudioSource() {
        factory = nullptr;
        source = nullptr;
        PeerConnectionFactory::UnRef();
    }

    MediaStreamTrack *RTCAudioSource::createTrack() const {
        return MediaStreamTrack::holder()->GetOrCreate(
            factory->factory()->CreateAudioTrack(rtc::CreateRandomUuid(), source.get())
        );
    }

    void RTCAudioSource::OnData(const RTCOnDataEvent &data, const int64_t absolute_capture_timestamp_ms) const
    {
        source->PushData(data, absolute_capture_timestamp_ms);
    }
} // wrtc