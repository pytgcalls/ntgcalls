//
// Created by Lauren on 15/03/24.
//
#pragma once
#include <ntgcalls/instances/call_interface.hpp>
#include <ntgcalls/instances/p2p_call.hpp>
#include <wrtc/interfaces/group_connection.hpp>
#include <wrtc/models/media_content.hpp>
#include <wrtc/utils/json.hpp>

namespace ntgcalls::instances {
    using wrtc::utils::json;

    class GroupCall: public CallInterface {
        wrtc::utils::synchronized_callback<void()> broadcast_timestamp_callback_;
        wrtc::utils::synchronized_callback<void(wrtc::models::SegmentPartRequest)> segment_part_request_callback_;

        static void update_remote_video_constraints(wrtc::interfaces::GroupConnection* conn);

    protected:
        std::shared_ptr<wrtc::interfaces::GroupConnection> presentation_connection_;

    public:
        explicit GroupCall(wrtc::utils::SafeThread& update_thread): CallInterface(update_thread) {}

        void stop() override;

        std::string init();

        virtual std::string init_presentation();

        virtual void connect(const std::string& json_data, bool is_presentation);

        uint32_t add_incoming_video(int64_t user_id, const std::string& endpoint, const std::vector<wrtc::models::SsrcGroup>& ssrc_group) const;

        bool remove_incoming_video(const std::string& endpoint) const;

        void stop_presentation(bool force = false);

        void set_stream_sources(media::StreamManager::Mode mode, const media::MediaDescription& config) const override;

        Type type() const override;

        void on_upgrade(const std::function<void(media::MediaState)>& callback) const;

        void send_broadcast_part(int64_t segment_id, int32_t part_id, wrtc::models::MediaSegment::Part::Status status, bool quality_update, const std::optional<bytes::binary>& data) const;

        void on_request_broadcast_part(const std::function<void(wrtc::models::SegmentPartRequest)>& callback);

        void send_broadcast_timestamp(int64_t timestamp) const;

        void on_request_broadcast_timestamp(const std::function<void()>& callback);
    };

} // ntgcalls::instances
