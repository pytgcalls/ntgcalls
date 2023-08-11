//
// Created by Laky64 on 09/08/2023.
//

#pragma once

#include <webrtc/api/ice_transport_interface.h>
#include <rtc_base/third_party/sigslot/sigslot.h>
#include "p2p/base/ice_transport_internal.h"

#include "rtc_peer_connection/peer_connection_factory.hpp"
#include "../utils/instance_holder.hpp"
#include "../enums/rtc_ice_component.hpp"


namespace wrtc {

    class RTCIceTransport : public sigslot::has_slots<sigslot::multi_threaded_local> {
    public:
        explicit RTCIceTransport(PeerConnectionFactory *, rtc::scoped_refptr<webrtc::IceTransportInterface>);
        static RTCIceTransport *Create(PeerConnectionFactory *, rtc::scoped_refptr<webrtc::IceTransportInterface>);

        ~RTCIceTransport() override;

        static InstanceHolder<
                RTCIceTransport *, rtc::scoped_refptr<webrtc::IceTransportInterface>, PeerConnectionFactory *
        > *holder();

        void OnRTCDtlsTransportStopped();

        RTCIceComponent GetComponent();

        cricket::IceGatheringState GetGatheringState();

        cricket::IceRole GetRole();

        webrtc::IceTransportState GetState();

    protected:
        void Stop();

    private:
        void OnStateChanged(cricket::IceTransportInternal *);

        void OnGatheringStateChanged(cricket::IceTransportInternal *);

        void TakeSnapshot();

        RTCIceComponent _component = RTCIceComponent::kRtp;
        PeerConnectionFactory *_factory;
        cricket::IceGatheringState _gathering_state = cricket::IceGatheringState::kIceGatheringNew;
        std::mutex _mutex{};
        cricket::IceRole _role = cricket::IceRole::ICEROLE_UNKNOWN;
        webrtc::IceTransportState _state = webrtc::IceTransportState::kNew;
        rtc::scoped_refptr<webrtc::IceTransportInterface> _transport;
    };

} // wrtc
