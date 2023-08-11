//
// Created by Laky64 on 09/08/2023.
//

#include "rtc_rtc_sender.hpp"

namespace wrtc {
    RTCRtpSender::RTCRtpSender(PeerConnectionFactory *factory, rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
            : _factory(factory), _sender(std::move(sender)) {}

    RTCRtpSender::~RTCRtpSender() {
        _factory = nullptr;

        holder()->Release(this);
    }

    InstanceHolder<RTCRtpSender *, rtc::scoped_refptr<webrtc::RtpSenderInterface>, PeerConnectionFactory *> *
    RTCRtpSender::holder() {
        static auto holder = new InstanceHolder<
                RTCRtpSender *, rtc::scoped_refptr<webrtc::RtpSenderInterface>, PeerConnectionFactory *
        >(RTCRtpSender::Create);
        return holder;
    }

    RTCRtpSender *RTCRtpSender::Create(
            PeerConnectionFactory *factory, rtc::scoped_refptr<webrtc::RtpSenderInterface> sender
    ) {
        return new RTCRtpSender(factory, std::move(sender));
    }

    std::optional<MediaStreamTrack *> RTCRtpSender::GetTrack() {
        auto track = _sender->track();
        if (track) {
            return MediaStreamTrack::holder()->GetOrCreate(_factory, track);
        }

        return {};
    }

    std::optional<RTCDtlsTransport *> RTCRtpSender::GetTransport() {
        auto transport = _sender->dtls_transport();
        if (transport) {
            return RTCDtlsTransport::holder()->GetOrCreate(_factory, transport);
        }

        return {};
    }
} // wrtc