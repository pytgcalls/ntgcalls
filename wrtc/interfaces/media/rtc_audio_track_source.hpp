//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <atomic>

#include <webrtc/pc/local_audio_source.h>

#include "../rtc_peer_connection/peer_connection_factory.hpp"
#include "../../models/rtc_on_data_event.hpp"

namespace wrtc {

  class RTCAudioTrackSource : public webrtc::LocalAudioSource {
  public:
    RTCAudioTrackSource() = default;

    ~RTCAudioTrackSource() override;

    SourceState state() const override;

    bool remote() const override;

    void PushData(RTCOnDataEvent &);

    void AddSink(webrtc::AudioTrackSinkInterface *) override;

    void RemoveSink(webrtc::AudioTrackSinkInterface *) override;

  private:
    PeerConnectionFactory *_factory = PeerConnectionFactory::GetOrCreateDefault();

    std::atomic<webrtc::AudioTrackSinkInterface *> _sink = {nullptr};
  };

} // namespace wrtc
