//
// Created by Laky64 on 09/08/2023.
//

#include "rtc_ice_transport.hpp"

namespace wrtc {
    RTCIceTransport::RTCIceTransport(
            PeerConnectionFactory *factory, rtc::scoped_refptr<webrtc::IceTransportInterface> transport) {
        _factory = factory;
        _transport = std::move(transport);

        _factory->_workerThread->Invoke<void>(RTC_FROM_HERE, [this]() {
            auto internal = _transport->internal();
            if (internal) {
                internal->SignalIceTransportStateChanged.connect(this, &RTCIceTransport::OnStateChanged);
                internal->SignalGatheringState.connect(this, &RTCIceTransport::OnGatheringStateChanged);
            }
            TakeSnapshot();
            if (_state == webrtc::IceTransportState::kClosed) {
                Stop();
            }
        });
    }

    RTCIceTransport::~RTCIceTransport() {
        _factory = nullptr;
        holder()->Release(this);
    }

    InstanceHolder<RTCIceTransport *, rtc::scoped_refptr<webrtc::IceTransportInterface>, PeerConnectionFactory *> *
    RTCIceTransport::holder() {
        static auto holder = new InstanceHolder<
                RTCIceTransport *, rtc::scoped_refptr<webrtc::IceTransportInterface>, PeerConnectionFactory *
        >(RTCIceTransport::Create);
        return holder;
    }

    RTCIceTransport *RTCIceTransport::Create(
            PeerConnectionFactory *factory, rtc::scoped_refptr<webrtc::IceTransportInterface> transport
    ) {
        // TODO: Needed to be clean from the memory
        return new RTCIceTransport(factory, std::move(transport));
    }

    void RTCIceTransport::TakeSnapshot() {
        std::lock_guard<std::mutex> lock(_mutex);
        auto internal = _transport->internal();
        if (internal) {
            if (internal->component() == 1) {
                _component = RTCIceComponent::kRtp;
            } else {
                _component = RTCIceComponent::kRtcp;
            }

            _role = internal->GetIceRole();
            _state = internal->GetIceTransportState();
            _gathering_state = internal->gathering_state();
        } else {
            _state = webrtc::IceTransportState::kClosed;
            _gathering_state = cricket::IceGatheringState::kIceGatheringComplete;
        }
    }

    void RTCIceTransport::OnRTCDtlsTransportStopped() {
        std::lock_guard<std::mutex> lock(_mutex);
        _state = webrtc::IceTransportState::kClosed;
        _gathering_state = cricket::IceGatheringState::kIceGatheringComplete;
        Stop();
    }

    void RTCIceTransport::Stop() {

    }

    void RTCIceTransport::OnStateChanged(cricket::IceTransportInternal *) {
        TakeSnapshot();

        // TODO call callback

        if (_state == webrtc::IceTransportState::kClosed) {
            Stop();
        }
    }

    void RTCIceTransport::OnGatheringStateChanged(cricket::IceTransportInternal *) {
        TakeSnapshot();

        // TODO call callback
    }

    RTCIceComponent RTCIceTransport::GetComponent() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_component == 1) {
            return RTCIceComponent::kRtp;
        } else {
            return RTCIceComponent::kRtcp;
        }
    }

    cricket::IceGatheringState RTCIceTransport::GetGatheringState() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _gathering_state;
    }

    cricket::IceRole RTCIceTransport::GetRole() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _role;
    }

    webrtc::IceTransportState RTCIceTransport::GetState() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _state;
    }
} // wrtc