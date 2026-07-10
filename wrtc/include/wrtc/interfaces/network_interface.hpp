//
// Created by Lauren on 29/03/24.
//

#pragma once
#include <wrtc/enums.hpp>
#include <wrtc/interfaces/media/remote_audio_sink.hpp>
#include <wrtc/interfaces/media/remote_video_sink.hpp>
#include <wrtc/interfaces/media/tracks/media_track_interface.hpp>
#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>
#include <wrtc/models/ice_candidate.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc::interfaces {

    class NetworkInterface {
    protected:
        peer_connection::PeerConnectionFactory* factory_;
        webrtc::Environment env_;
        utils::synchronized_callback<void()> data_channel_opened_callback_;
        utils::synchronized_callback<void(models::IceCandidate)> ice_candidate_callback_;
        utils::synchronized_callback<void(ConnectionState, bool)> connection_change_callback_;
        utils::synchronized_callback<void(bytes::binary)> data_channel_message_callback_;
        ConnectionState current_state_ = ConnectionState::Connecting;
        bool data_channel_open_ = false;
        bool already_connected_ = false;
        bool audio_incoming_ = false, camera_incoming_ = false, screen_incoming_ = false;

        static webrtc::IceCandidateInterface* parse_ice_candidate(const models::IceCandidate& raw_candidate);

    public:
        NetworkInterface();

        virtual void open() = 0;

        virtual ~NetworkInterface() = default;

        [[nodiscard]] utils::SafeThread& network_thread() const;

        [[nodiscard]] utils::SafeThread& signaling_thread() const;

        [[nodiscard]] utils::SafeThread& worker_thread() const;

        const webrtc::Environment& environment() const;

        void on_data_channel_opened(const std::function<void()> &callback);

        void on_ice_candidate(const std::function<void(const models::IceCandidate& candidate)>& callback);

        void on_connection_change(const std::function<void(ConnectionState state, bool was_connected)> &callback);

        void on_data_channel_message(const std::function<void(const bytes::binary& data)>& callback);

        virtual void close();

        virtual void send_data_channel_message(const bytes::binary &data) const = 0;

        virtual void add_ice_candidate(const models::IceCandidate& raw_candidate) const = 0;

        virtual std::unique_ptr<media::tracks::MediaTrackInterface> add_outgoing_track(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) = 0;

        virtual void add_incoming_audio_track(const std::weak_ptr<media::RemoteAudioSink>& sink) = 0;

        virtual void add_incoming_video_track(const std::weak_ptr<media::RemoteVideoSink>& sink, bool is_screen_cast) = 0;

        virtual ConnectionMode get_connection_mode() const = 0;

        bool is_data_channel_open() const;

        ConnectionState get_connection_state() const;

        virtual void enable_audio_incoming(bool enable);

        virtual void enable_video_incoming(bool enable, bool is_screen_cast);
    };

} // wrtc::interfaces
