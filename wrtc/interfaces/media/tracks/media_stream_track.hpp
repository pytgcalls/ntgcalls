//
// Created by Laky64 on 19/08/2023.
//

#pragma once

#include <api/media_stream_interface.h>
#include "../../../utils/instance_holder.hpp"

namespace wrtc {

    class MediaStreamTrack final : public webrtc::ObserverInterface {
    public:
        explicit MediaStreamTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);

        ~MediaStreamTrack() override;

        void Stop();

        void OnChanged() override;

        void OnPeerConnectionClosed();

        [[nodiscard]] bool isMuted() const;

        void Mute(bool);

        static InstanceHolder<MediaStreamTrack *, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>
        *holder();

        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track();

    private:
        bool _ended = false;
        bool _enabled = false;
        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> _track;

        static MediaStreamTrack *Create(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);
    };

} // wrtc
