//
// Created by Lauren on 15/03/24.
//

#include <ntgcalls/instances/call_interface.hpp>

namespace ntgcalls::instances {
    CallInterface::CallInterface(wrtc::utils::SafeThread& update_thread): update_thread_(update_thread) {
        stream_manager_ = std::make_shared<media::StreamManager>(update_thread);
    }

    void CallInterface::stop() {
        connection_change_callback_ = nullptr;
        remote_source_callback_ = nullptr;
        stream_manager_->close();
        stream_manager_ = nullptr;
        if (connection_) {
            connection_->close();
            connection_ = nullptr;
        }
    }

    wrtc::ConnectionMode CallInterface::get_connection_mode() const {
        return connection_->get_connection_mode();
    }

    bool CallInterface::pause() const {
        return stream_manager_->pause();
    }

    bool CallInterface::resume() const {
        return stream_manager_->resume();
    }

    bool CallInterface::mute() const {
        return stream_manager_->mute();
    }

    bool CallInterface::unmute() const {
        return stream_manager_->unmute();
    }

    void CallInterface::set_stream_sources(const media::StreamManager::Mode mode, const media::MediaDescription& config) const {
        stream_manager_->set_stream_sources(mode, config);
        if (mode == media::StreamManager::Mode::Playback && connection_) {
            stream_manager_->optimize_sources(connection_.get());
        }
    }

    void CallInterface::on_stream_end(const std::function<void(media::StreamManager::Type, media::StreamManager::Device)>& callback) const {
        stream_manager_->on_stream_end(callback);
    }

    void CallInterface::on_connection_change(const std::function<void(ConnectionInfo)>& callback) {
        connection_change_callback_ = callback;
    }

    void CallInterface::on_frames(const std::function<void(media::StreamManager::Mode, media::StreamManager::Device, const std::vector<wrtc::models::Frame>&)>& callback) const {
        stream_manager_->on_frames(callback);
    }

    void CallInterface::on_remote_source_change(const std::function<void(RemoteSource)>& callback) {
        remote_source_callback_ = callback;
    }

    uint64_t CallInterface::time(const media::StreamManager::Mode mode) const {
        return stream_manager_->time(mode);
    }

    media::MediaState CallInterface::get_state() const {
        return stream_manager_->get_state();
    }

    media::StreamManager::Status CallInterface::status(const media::StreamManager::Mode mode) const {
        return stream_manager_->status(mode);
    }

    void CallInterface::send_external_frame(const media::StreamManager::Device device, const bytes::binary& data, const wrtc::models::FrameData frame_data) const {
        stream_manager_->send_external_frame(device, data, frame_data);
    }

    std::shared_ptr<media::StreamManager> CallInterface::stream_manager() const {
        return stream_manager_;
    }

    void CallInterface::set_connection_observer(const std::shared_ptr<wrtc::interfaces::NetworkInterface>& conn, ConnectionInfo::Kind kind) {
        RTC_LOG(LS_VERBOSE) << "Connecting...";
        (void) connection_change_callback_({ConnectionInfo::State::Connecting, kind});
        const std::weak_ptr weak(shared_from_this());
        conn->on_connection_change([weak, kind, conn](const wrtc::ConnectionState state, bool was_connected) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->update_thread_.PostTask([weak, kind, conn, state, was_connected] {
                const auto strong_update = weak.lock();
                if (!strong_update) {
                    return;
                }
                switch (state) {
                case wrtc::ConnectionState::Connecting:
                    if (was_connected) {
                        RTC_LOG(LS_VERBOSE) << "Reconnecting...";
                    }
                    return;
                case wrtc::ConnectionState::Connected:
                    RTC_LOG(LS_VERBOSE) << "Connection established";
                    if (!was_connected && strong_update->stream_manager_) {
                        strong_update->stream_manager_->start();
                        RTC_LOG(LS_VERBOSE) << "Stream started";
                        (void) strong_update->connection_change_callback_({ConnectionInfo::State::Connected, kind});
                    }
                    break;
                case wrtc::ConnectionState::Disconnected:
                case wrtc::ConnectionState::Failed:
                case wrtc::ConnectionState::Closed:
                    if (conn) {
                        conn->on_connection_change(nullptr);
                    }
                    if (state == wrtc::ConnectionState::Failed) {
                        RTC_LOG(LS_ERROR) << "Connection failed";
                        (void) strong_update->connection_change_callback_({ConnectionInfo::State::Failed, kind});
                    } else {
                        RTC_LOG(LS_VERBOSE) << "Connection closed";
                        (void) strong_update->connection_change_callback_({ConnectionInfo::State::Closed, kind});
                    }
                    break;
                default:
                    break;
                }
            });
        });
        update_thread_.PostDelayedTask([weak, kind, conn] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            if (conn->get_connection_state() == wrtc::ConnectionState::Connecting && !conn->is_already_connected()) {
                RTC_LOG(LS_ERROR) << "Connection timeout";
                (void) strong->connection_change_callback_({ConnectionInfo::State::Timeout, kind});
            }
        }, webrtc::TimeDelta::Seconds(10));
    }

    media::StreamManager::Status CallInterface::parse_video_state(const signaling::messages::MediaStateMessage::VideoState state) {
        switch (state){
        case signaling::messages::MediaStateMessage::VideoState::Active:
            return media::StreamManager::Status::Active;
        case signaling::messages::MediaStateMessage::VideoState::Inactive:
            return media::StreamManager::Status::Idling;
        case signaling::messages::MediaStateMessage::VideoState::Suspended:
            return media::StreamManager::Status::Paused;
        }
        return media::StreamManager::Status::Idling;
    }
} // ntgcalls::instances