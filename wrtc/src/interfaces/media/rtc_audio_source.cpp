//
// Created by Laky64 on 19/08/2023.
//

#include <wrtc/interfaces/media/rtc_audio_source.hpp>
#include <rtc_base/crypto_random.h>

namespace wrtc {
    RTCAudioSource::RTCAudioSource() {
        factory = PeerConnectionFactory::GetOrCreateDefault();
        source = new webrtc::RefCountedObject<AudioTrackSource>();
    }

    RTCAudioSource::~RTCAudioSource() {
        factory = nullptr;
        source = nullptr;
    }

    webrtc::scoped_refptr<webrtc::AudioTrackInterface> RTCAudioSource::createTrack() const {
        return factory->factory()->CreateAudioTrack(webrtc::CreateRandomUuid(), source.get());
    }

    void RTCAudioSource::OnData(const RTCOnDataEvent &data, const FrameData additionalData) const {
        source->PushData(data, additionalData.absoluteCaptureTimestampMs);
    }
} // wrtc