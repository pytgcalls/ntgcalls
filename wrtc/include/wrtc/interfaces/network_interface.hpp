//
// Created by Laky64 on 29/03/2024.
//

#pragma once
#include <wrtc/interfaces/media/tracks/media_track_interface.hpp>
#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>
#include <wrtc/enums.hpp>
#include <wrtc/models/ice_candidate.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/syncronized_callback.hpp>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>
#include <wrtc/interfaces/media/remote_video_sink.hpp>

namespace wrtc {

    class NetworkInterface {
    protected:
        rtc::scoped_refptr<PeerConnectionFactory> factory;
        synchronized_callback<void> dataChannelOpenedCallback;
        synchronized_callback<IceCandidate> iceCandidateCallback;
        synchronized_callback<ConnectionState, bool> connectionChangeCallback;
        synchronized_callback<bytes::binary> dataChannelMessageCallback;
        bool dataChannelOpen = false;
        bool alreadyConnected = false;
        bool audioIncoming = false, cameraIncoming = false, screenIncoming = false;

        static webrtc::IceCandidateInterface* parseIceCandidate(const IceCandidate& rawCandidate);

    public:
        NetworkInterface();

        virtual void open() = 0;

        virtual ~NetworkInterface() = default;

        [[nodiscard]] rtc::Thread *networkThread() const;

        [[nodiscard]] rtc::Thread *signalingThread() const;

        [[nodiscard]] rtc::Thread *workerThread() const;

        const webrtc::Environment& environment() const;

        void onDataChannelOpened(const std::function<void()> &callback);

        void onIceCandidate(const std::function<void(const IceCandidate& candidate)>& callback);

        void onConnectionChange(const std::function<void(ConnectionState state, bool wasConnected)> &callback);

        void onDataChannelMessage(const std::function<void(const bytes::binary& data)>& callback);

        virtual void close();

        virtual void sendDataChannelMessage(const bytes::binary &data) const = 0;

        virtual void addIceCandidate(const IceCandidate& rawCandidate) const = 0;

        virtual std::unique_ptr<MediaTrackInterface> addOutgoingTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) = 0;

        virtual void addIncomingAudioTrack(const std::weak_ptr<RemoteAudioSink>& sink) = 0;

        virtual void addIncomingVideoTrack(const std::weak_ptr<RemoteVideoSink>& sink, bool isScreenCast) = 0;

        bool isDataChannelOpen() const;

        bool isConnected() const;

        virtual void enableAudioIncoming(bool enable);

        virtual void enableVideoIncoming(bool enable, bool isScreenCast);
    };

} // wrtc
