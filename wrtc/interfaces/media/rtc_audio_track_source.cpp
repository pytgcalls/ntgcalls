//
// Created by Laky64 on 08/08/2023.
//

#include "rtc_audio_track_source.hpp"

namespace wrtc {

  RTCAudioTrackSource::~RTCAudioTrackSource() {
    PeerConnectionFactory::Release();
    _factory = nullptr;
  }

  webrtc::MediaSourceInterface::SourceState RTCAudioTrackSource::state() const {
    return webrtc::MediaSourceInterface::SourceState::kLive;
  }

  bool RTCAudioTrackSource::remote() const {
    return false;
  }

  void RTCAudioTrackSource::AddSink(webrtc::AudioTrackSinkInterface *sink) {
    _sink = sink;
  }

  void RTCAudioTrackSource::RemoveSink(webrtc::AudioTrackSinkInterface *) {
    _sink = nullptr;
  }

  void RTCAudioTrackSource::PushData(RTCOnDataEvent &data) {
    webrtc::AudioTrackSinkInterface *sink = _sink;
    if (sink) {
      sink->OnData(
          data.audioData,
          data.bitsPerSample,
          data.sampleRate,
          data.channelCount,
          data.numberOfFrames
      );
    }
  }

} // namespace wrtc
