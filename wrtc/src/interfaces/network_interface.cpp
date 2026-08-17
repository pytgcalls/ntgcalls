//
// Created by Lauren on 29/03/24.
//

#include <wrtc/interfaces/network_interface.hpp>

#include <wrtc/exceptions.hpp>
#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>

namespace wrtc::interfaces {
    webrtc::IceCandidateInterface* NetworkInterface::parse_ice_candidate(const models::IceCandidate& raw_candidate) {
        webrtc::SdpParseError error;
        const auto candidate = CreateIceCandidate(raw_candidate.mid, raw_candidate.m_line, raw_candidate.sdp, &error);
        if (!candidate) {
            throw wrap_sdp_parse_error(error);
        }
        return candidate;
    }

    NetworkInterface::NetworkInterface(): env_(peer_connection::PeerConnectionFactory::environment()) {
        factory_ = peer_connection::PeerConnectionFactory::get_or_create_default();
    }

    utils::SafeThread& NetworkInterface::network_thread() const {
        return factory_->network_thread();
    }

    utils::SafeThread& NetworkInterface::signaling_thread() const {
        return factory_->signaling_thread();
    }

    utils::SafeThread& NetworkInterface::worker_thread() const {
        return factory_->worker_thread();
    }

    const webrtc::Environment& NetworkInterface::environment() const {
        return env_;
    }

    void NetworkInterface::on_data_channel_opened(const std::function<void()>& callback) {
        data_channel_opened_callback_ = callback;
    }

    void NetworkInterface::on_ice_candidate(const std::function<void(const models::IceCandidate& candidate)>& callback) {
        ice_candidate_callback_ = callback;
    }

    void NetworkInterface::on_connection_change(const std::function<void(ConnectionState state, bool was_connected)>& callback) {
        connection_change_callback_ = callback;
    }

    void NetworkInterface::on_data_channel_message(const std::function<void(const bytes::binary& data)>& callback) {
        data_channel_message_callback_ = callback;
    }

    void NetworkInterface::close() {
        data_channel_opened_callback_ = nullptr;
        ice_candidate_callback_ = nullptr;
        connection_change_callback_ = nullptr;
        data_channel_message_callback_ = nullptr;
        if (factory_) {
            factory_ = nullptr;
        }
    }

    bool NetworkInterface::is_data_channel_open() const {
        return data_channel_open_;
    }

    ConnectionState NetworkInterface::get_connection_state() const {
        return current_state_;
    }

    bool NetworkInterface::is_already_connected() const {
        return already_connected_;
    }

    void NetworkInterface::enable_audio_incoming(const bool enable) {
        audio_incoming_ = enable;
    }

    void NetworkInterface::enable_video_incoming(const bool enable, const bool is_screen_cast) {
        if (is_screen_cast) {
            screen_incoming_ = enable;
        } else {
            camera_incoming_ = enable;
        }
    }
} // wrtc::interfaces