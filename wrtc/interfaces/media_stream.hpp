//
// Created by Laky64 on 09/08/2023.
//

#pragma once

#include <vector>
#include "media_stream_track.hpp"

namespace wrtc {

    class MediaStream {
    public:
        explicit MediaStream();

        explicit MediaStream(MediaStream *);

        explicit MediaStream(std::vector<MediaStreamTrack *>);

        MediaStream(PeerConnectionFactory *, rtc::scoped_refptr<webrtc::MediaStreamInterface>);

        static MediaStream *Create(PeerConnectionFactory *, rtc::scoped_refptr<webrtc::MediaStreamInterface>);

        static InstanceHolder<
                MediaStream *, rtc::scoped_refptr<webrtc::MediaStreamInterface>, PeerConnectionFactory *
        > *holder();

        rtc::scoped_refptr<webrtc::MediaStreamInterface> stream();

        std::string GetId();

        bool GetActive();

        std::vector<MediaStreamTrack *> GetAudioTracks();

        std::vector<MediaStreamTrack *> GetVideoTracks();

        std::vector<MediaStreamTrack *> GetTracks();

        std::optional<MediaStreamTrack *> GetTrackById(const std::string &);

        void AddTrack(MediaStreamTrack &);

        void RemoveTrack(MediaStreamTrack &);

    private:
        class Impl {
        public:
            Impl &operator=(Impl &&other) noexcept {
                if (&other != this) {
                    _factory = other._factory;
                    other._factory = nullptr;
                    _stream = std::move(other._stream);
                    _shouldReleaseFactory = other._shouldReleaseFactory;
                    if (_shouldReleaseFactory) {
                        other._shouldReleaseFactory = false;
                    }
                }
                return *this;
            }

            explicit Impl(PeerConnectionFactory *factory = nullptr);

            explicit Impl(std::vector<MediaStreamTrack *> &&tracks, PeerConnectionFactory *factory = nullptr);

            explicit Impl(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream, PeerConnectionFactory *factory = nullptr);

//      TODO
//      Impl(const RTCMediaStreamInit& init, PeerConnectionFactory* factory = nullptr);

            ~Impl();

            PeerConnectionFactory *_factory;
            rtc::scoped_refptr<webrtc::MediaStreamInterface> _stream;
            bool _shouldReleaseFactory;
        };

        std::vector<rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> tracks();

        Impl _impl;
    };
} // wrtc
