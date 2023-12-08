//
// Created by Laky64 on 19/08/2023.
//

#include "media_stream_track.hpp"

namespace wrtc {

    MediaStreamTrack::MediaStreamTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) {
        _track = track;
        _track->RegisterObserver(this);
    }

    MediaStreamTrack::~MediaStreamTrack() {
        _track = nullptr;

        holder()->Release(this);
    }

    void MediaStreamTrack::OnChanged() {
        if (_track->state() == webrtc::MediaStreamTrackInterface::TrackState::kEnded) {
            Stop();
        }
    }

    void MediaStreamTrack::Stop() {
        _track->UnregisterObserver(this);
        _ended = true;
        _enabled = _track->enabled();
    }

    void MediaStreamTrack::OnPeerConnectionClosed() {
        Stop();
    }

    bool MediaStreamTrack::isMuted() const
    {
        return _ended ? !_enabled : !_track->enabled();
    }

    void MediaStreamTrack::Mute(const bool muted) {
        if (_ended) {
            _enabled = !muted;
        } else {
            _track->set_enabled(!muted);
        }
    }

    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> MediaStreamTrack::track() {
        return _track;
    }

    InstanceHolder<MediaStreamTrack *, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> *
    MediaStreamTrack::holder() {
        return new InstanceHolder<
            MediaStreamTrack*, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        >(Create);
    }

    MediaStreamTrack *MediaStreamTrack::Create(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track) {
        return new MediaStreamTrack(track);
    }

} // wrtc