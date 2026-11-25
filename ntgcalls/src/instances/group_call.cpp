//
// Created by Lauren on 15/03/24.
//

#include <ntgcalls/instances/group_call.hpp>

#include <future>
#include <ntgcalls/exceptions.hpp>
#include <wrtc/interfaces/group_connection.hpp>
#include <wrtc/interfaces/response_payload.hpp>

#define RTMP_UNSUPPORTED_THROW                                         \
    RTC_LOG(LS_ERROR) << "Streaming is not supported when using RTMP"; \
    throw RTMPStreamingUnsupported("Streaming is not supported when using RTMP");

namespace ntgcalls::instances {

    void GroupCall::stop() {
        broadcast_timestamp_callback_ = nullptr;
        segment_part_request_callback_ = nullptr;
        {
            const std::lock_guard lock(lifecycle_mutex_);
            stopped_ = true;
        }
        stop_presentation();
        CallInterface::stop();
    }

    std::string GroupCall::init() {
        RTC_LOG(LS_INFO) << "Initializing group call";
        if (connection()) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        stream_manager_->set_stream_sources(media::StreamManager::Mode::Capture);
        stream_manager_->set_stream_sources(media::StreamManager::Mode::Playback);
        return open_connection();
    }

    std::string GroupCall::rejoin() {
        RTC_LOG(LS_INFO) << "Rejoining group call";
        const std::lock_guard lock(lifecycle_mutex_);
        if (stopped_) {
            RTC_LOG(LS_ERROR) << "Connection closed";
            throw ConnectionNotFound("Connection closed");
        }
        close_presentation();
        stream_manager_->detach();
        if (const auto previous = connection()) {
            previous->on_connection_change(nullptr);
            previous->close();
        }
        return open_connection();
    }

    std::string GroupCall::open_connection() {
        const auto conn = std::make_shared<wrtc::interfaces::GroupConnection>(false, type() == Type::Conference);
        conn->open();
        set_connection(conn);
        on_connection_opened(conn.get());
        RTC_LOG(LS_INFO) << "Group call initialized";
        stream_manager_->optimize_sources(conn.get());

        const std::weak_ptr weak(shared_from_this());
        conn->on_data_channel_opened([weak] {
            const auto strong = std::static_pointer_cast<GroupCall>(weak.lock());
            if (!strong) {
                return;
            }
            RTC_LOG(LS_VERBOSE) << "Data channel opened";
            if (const auto current = strong->connection()) {
                update_remote_video_constraints(safe<wrtc::interfaces::GroupConnection>(current));
            }
        });
        stream_manager_->add_track(media::StreamManager::Mode::Capture, media::StreamManager::Device::Microphone, conn.get());
        stream_manager_->add_track(media::StreamManager::Mode::Capture, media::StreamManager::Device::Camera, conn.get());
        stream_manager_->add_track(media::StreamManager::Mode::Playback, media::StreamManager::Device::Microphone, conn.get());
        stream_manager_->add_track(media::StreamManager::Mode::Playback, media::StreamManager::Device::Camera, conn.get());
        stream_manager_->add_track(media::StreamManager::Mode::Playback, media::StreamManager::Device::Screen, conn.get());
        RTC_LOG(LS_INFO) << "AVStream settings applied";
        return conn->get_join_payload();
    }

    void GroupCall::on_connection_opened(wrtc::interfaces::GroupConnection*) {}

    std::string GroupCall::init_presentation() {
        const std::lock_guard lock(lifecycle_mutex_);
        if (stopped_) {
            RTC_LOG(LS_ERROR) << "Connection closed";
            throw ConnectionNotFound("Connection closed");
        }
        if (connection_->get_connection_mode() != wrtc::ConnectionMode::Rtc) {
            RTC_LOG(LS_ERROR) << "Presentation connection requires RTC connection";
            throw RTCConnectionNeeded("Presentation connection requires RTC connection");
        }
        RTC_LOG(LS_INFO) << "Initializing screen sharing";
        const auto presentation = std::make_shared<wrtc::interfaces::GroupConnection>(true, type() == Type::Conference);
        {
            const std::lock_guard presentation_lock(presentation_mutex_);
            if (presentation_connection_) {
                RTC_LOG(LS_ERROR) << "Screen sharing already initialized";
                throw ConnectionError("Screen sharing already initialized");
            }
            presentation_connection_ = presentation;
        }
        presentation->open();
        stream_manager_->optimize_sources(presentation.get());
        const std::weak_ptr weak(shared_from_this());
        presentation->on_data_channel_opened([weak] {
            const auto strong = std::static_pointer_cast<GroupCall>(weak.lock());
            if (!strong) {
                return;
            }
            RTC_LOG(LS_VERBOSE) << "Data channel opened";
            if (const auto current = strong->presentation_connection()) {
                update_remote_video_constraints(current.get());
            }
        });
        stream_manager_->add_track(media::StreamManager::Mode::Capture, media::StreamManager::Device::Speaker, presentation.get());
        stream_manager_->add_track(media::StreamManager::Mode::Capture, media::StreamManager::Device::Screen, presentation.get());
        RTC_LOG(LS_INFO) << "Screen sharing initialized";
        return presentation->get_join_payload();
    }

    void GroupCall::connect(const std::string& json_data, const bool is_presentation) {
        RTC_LOG(LS_VERBOSE) << "Connecting to group call";
        const std::lock_guard lock(lifecycle_mutex_);
        if (stopped_) {
            RTC_LOG(LS_ERROR) << "Connection closed";
            throw ConnectionNotFound("Connection closed");
        }
        const auto conn = is_presentation ? presentation_connection() : connection();
        if (!conn) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionError("Connection not initialized");
        }

        wrtc::interfaces::ResponsePayload payload(json_data);
        wrtc::ConnectionMode connection_mode;
        if (payload.is_rtmp) {
            connection_mode = wrtc::ConnectionMode::Rtmp;
        } else if (payload.is_stream) {
            connection_mode = wrtc::ConnectionMode::Stream;
        } else {
            connection_mode = wrtc::ConnectionMode::Rtc;
        }

        const auto current_connection_mode = conn->get_connection_mode();
        if (current_connection_mode == connection_mode || current_connection_mode == wrtc::ConnectionMode::Rtmp) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }

        if (current_connection_mode == wrtc::ConnectionMode::Rtc && connection_mode != wrtc::ConnectionMode::Stream) {
            RTC_LOG(LS_ERROR) << "Cannot switch connection mode from RTC to MTProto";
            throw ConnectionError("Cannot switch connection mode from RTC to MTProto");
        }

        if (connection_mode == wrtc::ConnectionMode::Rtmp && stream_manager_->has_readers()) {
            RTMP_UNSUPPORTED_THROW
        }

        safe<wrtc::interfaces::GroupConnection>(conn)->set_connection_mode(connection_mode);
        if (connection_mode == wrtc::ConnectionMode::Rtc) {
            safe<wrtc::interfaces::GroupConnection>(conn)->set_remote_params(payload.remote_ice_parameters, std::move(payload.fingerprint));
            for (const auto& raw_candidate : payload.candidates) {
                const webrtc::JsepIceCandidate ice_candidate{std::string(), 0, raw_candidate};
                conn->add_ice_candidate(wrtc::models::IceCandidate(&ice_candidate));
            }
            if (is_presentation) {
                const auto media_config = safe<wrtc::interfaces::GroupConnection>(conn)->get_media_config();
                payload.media.audio_payload_types = media_config.audio_payload_types;
                payload.media.audio_rtp_extensions = media_config.audio_rtp_extensions;
            }
            stream_manager_->optimize_sources(conn.get());
            safe<wrtc::interfaces::GroupConnection>(conn)->create_channels(payload.media);
            RTC_LOG(LS_VERBOSE) << "Remote parameters set";
        } else {
            const std::weak_ptr weak(shared_from_this());
            safe<wrtc::interfaces::GroupConnection>(conn)->on_request_broadcast_part([weak](const wrtc::models::SegmentPartRequest& request) {
                const auto strong = std::static_pointer_cast<GroupCall>(weak.lock());
                if (!strong) {
                    return;
                }
                (void) strong->segment_part_request_callback_(request);
            });
            safe<wrtc::interfaces::GroupConnection>(conn)->on_request_broadcast_timestamp([weak] {
                const auto strong = std::static_pointer_cast<GroupCall>(weak.lock());
                if (!strong) {
                    return;
                }
                (void) strong->broadcast_timestamp_callback_();
            });
            safe<wrtc::interfaces::GroupConnection>(conn)->connect_media_stream();
            stream_manager_->optimize_sources(conn.get());
            RTC_LOG(LS_VERBOSE) << "MTProto stream attached";
        }
        set_connection_observer(
            conn,
            is_presentation ? ConnectionInfo::Kind::Presentation : ConnectionInfo::Kind::Normal
        );
    }

    void GroupCall::update_remote_video_constraints(wrtc::interfaces::GroupConnection* conn) {
        json json_res = {
            {"colibriClass", "ReceiverVideoConstraints"},
            {"constraints", json::object()},
            {"defaultConstraints", {{"maxHeight", 0}}},
            {"onStageEndpoints", json::array()}
        };
        for (const auto& endpoint : conn->get_endpoints()) {
            json_res["constraints"][endpoint] = {
                {"maxHeight", 720},
                {"minHeight", 180},
            };
        }
        conn->send_data_channel_message(bytes::make_binary(json_res.dump()));
    }

    uint32_t GroupCall::add_incoming_video(const int64_t user_id, const std::string& endpoint, const std::vector<wrtc::models::SsrcGroup>& ssrc_group) const {
        const auto current = connection();
        if (!current) {
            throw ConnectionError("Connection not initialized");
        }
        const auto conn = safe<wrtc::interfaces::GroupConnection>(current);
        const auto ssrc = conn->add_incoming_video(user_id, endpoint, ssrc_group);
        if (get_connection_mode() == wrtc::ConnectionMode::Rtc) update_remote_video_constraints(conn);
        return ssrc;
    }

    bool GroupCall::remove_incoming_video(const std::string& endpoint) const {
        const auto current = connection();
        if (!current) {
            throw ConnectionError("Connection not initialized");
        }
        return safe<wrtc::interfaces::GroupConnection>(current)->remove_incoming_video(endpoint);
    }

    void GroupCall::stop_presentation(const bool force) {
        const std::lock_guard lifecycle_lock(lifecycle_mutex_);
        if (!close_presentation() && force) {
            throw ConnectionError("Presentation not initialized");
        }
    }

    bool GroupCall::close_presentation() {
        std::shared_ptr<wrtc::interfaces::GroupConnection> presentation;
        {
            const std::lock_guard lock(presentation_mutex_);
            presentation = std::move(presentation_connection_);
        }
        if (!presentation) {
            return false;
        }
        presentation->close();
        return true;
    }

    std::shared_ptr<wrtc::interfaces::GroupConnection> GroupCall::presentation_connection() const {
        const std::lock_guard lock(presentation_mutex_);
        return presentation_connection_;
    }

    void GroupCall::set_stream_sources(const media::StreamManager::Mode mode, const media::MediaDescription& config) const {
        if (mode == media::StreamManager::Mode::Capture && get_connection_mode() == wrtc::ConnectionMode::Rtmp) {
            RTMP_UNSUPPORTED_THROW
        }
        CallInterface::set_stream_sources(mode, config);
        if (const auto current = connection(); mode == media::StreamManager::Mode::Playback && current && current->get_connection_mode() == wrtc::ConnectionMode::Rtc) {
            update_remote_video_constraints(safe<wrtc::interfaces::GroupConnection>(current));
        }
        if (const auto presentation = presentation_connection(); mode == media::StreamManager::Mode::Playback && presentation) {
            stream_manager_->optimize_sources(presentation.get());
        }
    }

    void GroupCall::on_upgrade(const std::function<void(media::MediaState)>& callback) const {
        stream_manager_->on_upgrade(callback);
    }

    void GroupCall::send_broadcast_part(const int64_t segment_id, const int32_t part_id, const wrtc::models::MediaSegment::Part::Status status, const bool quality_update, const std::optional<bytes::binary>& data) const {
        const auto current = connection();
        if (!current) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionError("Connection not initialized");
        }
        safe<wrtc::interfaces::GroupConnection>(current)->send_broadcast_part(segment_id, part_id, status, quality_update, data);
    }

    void GroupCall::on_request_broadcast_part(const std::function<void(wrtc::models::SegmentPartRequest)>& callback) {
        segment_part_request_callback_ = callback;
    }

    void GroupCall::send_broadcast_timestamp(const int64_t timestamp) const {
        const auto current = connection();
        if (!current) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionError("Connection not initialized");
        }
        safe<wrtc::interfaces::GroupConnection>(current)->send_broadcast_timestamp(timestamp);
    }

    void GroupCall::on_request_broadcast_timestamp(const std::function<void()>& callback) {
        broadcast_timestamp_callback_ = callback;
    }

    CallInterface::Type GroupCall::type() const {
        return Type::Group;
    }
} // ntgcalls::instances
