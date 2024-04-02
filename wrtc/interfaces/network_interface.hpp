//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include "media/tracks/media_stream_track.hpp"
#include "peer_connection/peer_connection_factory.hpp"
#include "wrtc/enums.hpp"
#include "wrtc/models/ice_candidate.hpp"
#include "wrtc/utils/binary.hpp"
#include "wrtc/utils/syncronized_callback.hpp"

namespace wrtc {

    class NetworkInterface {
    protected:
        rtc::scoped_refptr<PeerConnectionFactory> factory;
        synchronized_callback<void> dataChannelOpenedCallback;
        synchronized_callback<IceCandidate> iceCandidateCallback;
        synchronized_callback<ConnectionState> connectionChangeCallback;
        bool dataChannelOpen = false;

        static webrtc::IceCandidateInterface* parseIceCandidate(const IceCandidate& rawCandidate);

    public:
        NetworkInterface();

        virtual ~NetworkInterface() = default;

        [[nodiscard]] rtc::Thread *networkThread() const;

        [[nodiscard]] rtc::Thread *signalingThread() const;

        [[nodiscard]] rtc::Thread *workerThread() const;

        void onDataChannelOpened(const std::function<void()> &callback);

        void onIceCandidate(const std::function<void(const IceCandidate& candidate)>& callback);

        void onConnectionChange(const std::function<void(ConnectionState state)> &callback);

        virtual void close();

        virtual void sendDataChannelMessage(const bytes::binary &data) const = 0;

        virtual void addIceCandidate(const IceCandidate& rawCandidate) const = 0;

        virtual void addTrack(MediaStreamTrack *mediaStreamTrack) = 0;

        bool isDataChannelOpen() const;
    };

} // wrtc
