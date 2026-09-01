//
// Created by Lauren on 12/04/25.
//

#pragma once
#include <atomic>
#include <shared_mutex>
#include <wrtc/interfaces/mtproto/thread_buffer.hpp>
#include <wrtc/models/audio_frame.hpp>
#include <wrtc/models/media_segment.hpp>
#include <wrtc/models/segment_part_request.hpp>
#include <wrtc/utils/safe_thread.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc::interfaces::mtproto {

    class MTProtoStream: public std::enable_shared_from_this<MTProtoStream> {
        struct AudioBuffer {
            uint32_t ssrc;
            std::vector<int16_t> data;
            size_t sample_rate = 0;
            int channels = 0;
        };

        struct VideoChannel {
            uint32_t ssrc;
            bool is_screen_cast;
            models::MediaSegment::Quality quality;
        };

        bool is_rtmp_;
        std::atomic_bool audio_incoming_ = false;
        std::atomic_bool camera_incoming_ = false;
        std::atomic_bool screen_incoming_ = false;
        bool is_waiting_current_time_ = false;
        const int segment_buffer_duration_ = 2000;
        const int segment_duration_ = 1000;
        int next_pending_request_time_delay_task_id_ = 0;
        int pending_request_time_delay_task_id_ = 0;
        int64_t next_segment_timestamp_ = -1;
        int64_t server_time_ms_ = 0;
        int64_t server_time_ms_got_at_ = 0;
        utils::SafeThread& media_thread_;
        std::atomic_bool running_;

        AudioStreamingPartPersistentDecoder persistent_audio_decoder_;
        std::optional<int> wait_for_buffered_milliseconds_before_rendering_;
        std::map<int64_t, std::unique_ptr<models::MediaSegment>> segments_;
        std::map<std::string, VideoChannel> video_channels_;
        std::map<std::string, int32_t> current_endpoint_mapping_;
        std::map<std::string, std::unique_ptr<VideoStreamingSharedState>> shared_video_state_;

        utils::synchronized_callback<void()> request_current_time_callback_;
        utils::synchronized_callback<void(int)> update_audio_source_count_callback_;
        utils::synchronized_callback<void(std::unique_ptr<models::AudioFrame>)> audio_frame_callback_;
        utils::synchronized_callback<void(uint32_t, bool, std::unique_ptr<webrtc::VideoFrame>)> video_frame_callback_;
        utils::synchronized_callback<void(models::SegmentPartRequest)> request_broadcast_part_callback_;

        std::shared_mutex segment_mutex_;
        std::unique_ptr<ThreadBuffer> thread_buffer_;

        std::map<int64_t, models::MediaSegment*> filter_segments(models::MediaSegment::Status status) const;

        void render();

        int64_t get_available_buffer_duration() const;

        void request_segments_if_needed();

        void check_pending_segments();

        void discard_all_pending_segments();

        static void cancel_pending_video_quality_update(models::MediaSegment::Video* segment);

        void check_pending_video_quality_update();

        void request_pending_video_quality_update(int64_t segment_id, int32_t part_id, models::MediaSegment::Video* segment, int64_t timestamp);

    public:
        explicit MTProtoStream(utils::SafeThread& media_thread, bool is_rtmp);

        void connect();

        void close();

        void send_broadcast_timestamp(int64_t timestamp);

        void send_broadcast_part(int64_t segment_id, int32_t part_id, models::MediaSegment::Part::Status status, bool quality_update, std::optional<bytes::binary> data);

        void add_incoming_video(const std::string& endpoint, uint32_t ssrc, bool is_screen_cast);

        bool remove_incoming_video(const std::string& endpoint);

        void enable_audio_incoming(bool enable);

        void enable_video_incoming(bool enable, bool is_screen_cast);

        void on_request_broadcast_time(const std::function<void()>& callback);

        void on_request_broadcast_part(const std::function<void(models::SegmentPartRequest)>& callback);

        void on_audio_frame(const std::function<void(std::unique_ptr<models::AudioFrame>)>& callback);

        void on_video_frame(const std::function<void(uint32_t, bool, std::unique_ptr<webrtc::VideoFrame>)>& callback);

        void on_update_audio_source_count(const std::function<void(int)>& callback);
    };
} // wrtc::interfaces::mtproto
