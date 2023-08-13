//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <webrtc/media/base/adapted_video_track_source.h>
#include <wrtc/interfaces/rtc_peer_connection/peer_connection_factory.hpp>


namespace wrtc {
    class RTCVideoTrackSource : public rtc::AdaptedVideoTrackSource {
    public:
        RTCVideoTrackSource();

        RTCVideoTrackSource(bool is_screencast, absl::optional<bool> needs_denoising);

        ~RTCVideoTrackSource() override;

        SourceState state() const override;

        bool remote() const override;

        bool is_screencast() const override;

        absl::optional<bool> needs_denoising() const override;

        void PushFrame(const webrtc::VideoFrame& frame);

    private:
        PeerConnectionFactory *_factory = PeerConnectionFactory::GetOrCreateDefault();

        const bool _is_screencast;
        const absl::optional<bool> _needs_denoising;
    };

} // wrtc
