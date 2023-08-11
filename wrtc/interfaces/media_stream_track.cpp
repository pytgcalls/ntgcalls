//
// Created by Laky64 on 09/08/2023.
//

#include "media_stream_track.hpp"

namespace wrtc {
    MediaStreamTrack::MediaStreamTrack(PeerConnectionFactory *factory,
                                       rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track) {
        _factory = factory;

        _track = std::move(track);
        _track->RegisterObserver(this);

        // TODO mb remove
        _enabled = false;
    }

    MediaStreamTrack::~MediaStreamTrack() {
        _track = nullptr;
        _factory = nullptr;

        holder()->Release(this);
    }

    void MediaStreamTrack::Stop() {
        _track->UnregisterObserver(this);
        _ended = true;
        _enabled = _track->enabled();
    }

    void MediaStreamTrack::OnChanged() {
        if (_track->state() == webrtc::MediaStreamTrackInterface::TrackState::kEnded) {
            Stop();
        }
    }

    void MediaStreamTrack::OnPeerConnectionClosed() {
        Stop();
    }

    bool MediaStreamTrack::GetEnabled() {
        return _ended ? _enabled : _track->enabled();
    }

    void MediaStreamTrack::SetEnabled(bool enabled) {
        if (_ended) {
            _enabled = enabled;
        } else {
            _track->set_enabled(enabled);
        }
    }

    std::string MediaStreamTrack::GetId() {
        return _track->id();
    }

    cricket::MediaType MediaStreamTrack::GetKind() {
        if (_track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
            return cricket::MediaType::MEDIA_TYPE_AUDIO;
        } else if (_track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
            return cricket::MediaType::MEDIA_TYPE_VIDEO;
        }

        return cricket::MediaType::MEDIA_TYPE_UNSUPPORTED;
    }

    webrtc::MediaStreamTrackInterface::TrackState MediaStreamTrack::GetReadyState() {
        auto state = _ended
                     ? webrtc::MediaStreamTrackInterface::TrackState::kEnded
                     : _track->state();
        return state;
    }

    bool MediaStreamTrack::GetMuted() {
        return false;
    }

    MediaStreamTrack *MediaStreamTrack::Clone() {
        auto label = rtc::CreateRandomUuid();
        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> clonedTrack = nullptr;

        if (_track->kind() == _track->kAudioKind) {
            auto audioTrack = dynamic_cast<webrtc::AudioTrackInterface *>(_track.get());
            clonedTrack = _factory->factory()->CreateAudioTrack(label, audioTrack->GetSource());
        } else {
            auto videoTrack = dynamic_cast<webrtc::VideoTrackInterface *>(_track.get());
            clonedTrack = _factory->factory()->CreateVideoTrack(label, videoTrack->GetSource());
        }

        auto clonedMediaStreamTrack = holder()->GetOrCreate(_factory, clonedTrack);
        if (_ended) {
            clonedMediaStreamTrack->Stop();
        }
        return clonedMediaStreamTrack;
    }

    MediaStreamTrack::operator rtc::scoped_refptr<webrtc::AudioTrackInterface>() {
        return {dynamic_cast<webrtc::AudioTrackInterface *>(_track.get())};
    }

    MediaStreamTrack::operator rtc::scoped_refptr<webrtc::VideoTrackInterface>() {
        return {dynamic_cast<webrtc::VideoTrackInterface *>(_track.get())};
    }

    InstanceHolder<MediaStreamTrack *, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>, PeerConnectionFactory *> *
    MediaStreamTrack::holder() {
        static auto holder = new wrtc::InstanceHolder<
                MediaStreamTrack *, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>, PeerConnectionFactory *
        >(MediaStreamTrack::Create);
        return holder;
    }

    MediaStreamTrack *MediaStreamTrack::Create(PeerConnectionFactory *factory,
                                               rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track) {
        // TODO: Needed to be clean from the memory
        return new MediaStreamTrack(factory, std::move(track));
    }
} // wrtc