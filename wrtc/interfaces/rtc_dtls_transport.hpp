//
// Created by Laky64 on 09/08/2023.
//

#pragma once

#include <api/dtls_transport_interface.h>

#include "rtc_peer_connection/peer_connection_factory.hpp"
#include "../utils/instance_holder.hpp"
#include "rtc_ice_transport.hpp"

namespace wrtc {

    class RTCDtlsTransport : public webrtc::DtlsTransportObserverInterface {
    public:
        explicit RTCDtlsTransport(PeerConnectionFactory *, rtc::scoped_refptr<webrtc::DtlsTransportInterface>);

        static RTCDtlsTransport *Create(PeerConnectionFactory *, rtc::scoped_refptr<webrtc::DtlsTransportInterface>);

        ~RTCDtlsTransport() override;

        static InstanceHolder<
                RTCDtlsTransport *, rtc::scoped_refptr<webrtc::DtlsTransportInterface>, PeerConnectionFactory *
        > *holder();

        void OnStateChange(webrtc::DtlsTransportInformation) override;

        void OnError(webrtc::RTCError) override;

        RTCIceTransport *GetIceTransport();

        webrtc::DtlsTransportState GetState();

    protected:
        void Stop();

    private:
        std::mutex _mutex;
        webrtc::DtlsTransportState _state;
        std::vector<rtc::Buffer> _certificates;

        PeerConnectionFactory *_factory;
        rtc::scoped_refptr<webrtc::DtlsTransportInterface> _transport;
    };

} // wrtc
