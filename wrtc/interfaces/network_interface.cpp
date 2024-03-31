//
// Created by Laky64 on 29/03/2024.
//

#include "network_interface.hpp"

#include "peer_connection/peer_connection_factory.hpp"
#include "wrtc/exceptions.hpp"

namespace wrtc {
    webrtc::IceCandidateInterface* NetworkInterface::parseIceCandidate(const IceCandidate& rawCandidate) {
        webrtc::SdpParseError error;
        const auto candidate = CreateIceCandidate(rawCandidate.mid, rawCandidate.mLine, rawCandidate.sdp, &error);
        if (!candidate) {
            throw wrapSdpParseError(error);
        }
        return candidate;
    }

    NetworkInterface::NetworkInterface() {
        factory = PeerConnectionFactory::GetOrCreateDefault();
    }

    rtc::Thread* NetworkInterface::networkThread() const {
        return factory->networkThread();
    }

    rtc::Thread* NetworkInterface::signalingThread() const {
        return factory->signalingThread();
    }

    rtc::Thread* NetworkInterface::workerThread() const {
        return factory->workerThread();
    }

    void NetworkInterface::onDataChannelOpened(const std::function<void()>& callback) {
        dataChannelOpenedCallback = callback;
    }

    void NetworkInterface::onIceCandidate(const std::function<void(const IceCandidate& candidate)>& callback) {
        iceCandidateCallback = callback;
    }

    void NetworkInterface::onConnectionChange(const std::function<void(ConnectionState state)>& callback) {
        connectionChangeCallback = callback;
    }

    void NetworkInterface::close() {
        if (factory) {
            PeerConnectionFactory::UnRef();
            factory = nullptr;
        }
    }

    bool NetworkInterface::isDataChannelOpen() const {
        return dataChannelOpen;
    }
} // wrtc