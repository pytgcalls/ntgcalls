//
// Created by Laky64 on 09/08/2023.
//

#pragma once

#include <api/media_stream_interface.h>
#include <api/scoped_refptr.h>

#include "rtc_peer_connection/peer_connection_factory.hpp"
#include "../utils/instance_holder.hpp"

namespace wrtc {

    class MediaStreamTrack : public webrtc::ObserverInterface {
    private:
        bool _ended = false;
        bool _enabled;
        PeerConnectionFactory *_factory;
        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> _track;

    public:
        explicit MediaStreamTrack(PeerConnectionFactory *, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>);

        static MediaStreamTrack *Create(
                PeerConnectionFactory *factory, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);

        ~MediaStreamTrack() override;

        static InstanceHolder<
                MediaStreamTrack *, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>, PeerConnectionFactory *
        > *holder();

        void Stop();

        // ObserverInterface
        void OnChanged() override;

        void OnPeerConnectionClosed();

        bool GetEnabled();

        void SetEnabled(bool);

        std::string GetId();

        cricket::MediaType GetKind();

        webrtc::MediaStreamTrackInterface::TrackState GetReadyState();

        bool GetMuted();

        // should be returned to python as reference! because we holding it in our holder
        MediaStreamTrack *Clone();

        bool active() { return !_ended && _track->state() == webrtc::MediaStreamTrackInterface::TrackState::kLive; }

        PeerConnectionFactory *factory() { return _factory; }

        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track() { return _track; }

        explicit operator rtc::scoped_refptr<webrtc::AudioTrackInterface>();

        explicit operator rtc::scoped_refptr<webrtc::VideoTrackInterface>();
    };

} // wrtc
