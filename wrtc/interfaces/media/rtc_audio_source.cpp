//
// Created by Laky64 on 08/08/2023.
//

#include "rtc_audio_source.hpp"

namespace wrtc {

  RTCAudioSource::RTCAudioSource() {
    _source = new rtc::RefCountedObject<RTCAudioTrackSource>();
  }

  MediaStreamTrack *RTCAudioSource::CreateTrack() {
    // TODO(mroberts): Again, we have some implicit factory we are threading around. How to handle?
    auto factory = PeerConnectionFactory::GetOrCreateDefault();
    auto track = factory->factory()->CreateAudioTrack(rtc::CreateRandomUuid(), _source.get());
    return MediaStreamTrack::holder()->GetOrCreate(factory, track);
  }

  void RTCAudioSource::OnData(RTCOnDataEvent &data) {
    _source->PushData(data);
  }

} // namespace wrtc
