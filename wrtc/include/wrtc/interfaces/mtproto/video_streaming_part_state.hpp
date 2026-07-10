//
// Created by Lauren on 15/04/25.
//

#pragma once
#include <optional>
#include <api/media_types.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part.hpp>
#include <wrtc/interfaces/mtproto/video_streaming_part_internal.hpp>

namespace wrtc::interfaces::mtproto {

    class VideoStreamingPartState {
        struct StreamEvent {
            int32_t offset = 0;
            std::string endpoint_id;
            int32_t rotation = 0;
            int32_t extra = 0;
        };

        struct StreamInfo {
            std::string container;
            int32_t active_mask = 0;
            std::vector<StreamEvent> events;
        };

        std::optional<StreamInfo> stream_info_;
        std::vector<media::VideoStreamingPartFrame> available_frames_;
        std::vector<std::unique_ptr<AudioStreamingPart>> parsed_audio_parts_;
        std::vector<std::unique_ptr<VideoStreamingPartInternal>> parsed_video_parts_;

        static int32_t round_up(int32_t num_to_round);

        static std::optional<int32_t> read_int32(const bytes::binary &data, int &offset);

        static std::optional<uint8_t> read_bytes_as_int32(const bytes::binary &data, int &offset, int count);

        static std::optional<std::string> read_serialized_string(const bytes::binary &data, int &offset);

        static std::optional<StreamEvent> read_video_stream_event(const bytes::binary &data, int &offset);

        static std::optional<StreamInfo> consume_stream_info(bytes::binary &data);

    public:
        explicit VideoStreamingPartState(bytes::binary&& data, webrtc::MediaType media_type);

        ~VideoStreamingPartState();

        std::optional<media::VideoStreamingPartFrame> get_frame_at_relative_timestamp(VideoStreamingSharedState* shared_state, double timestamp);

        [[nodiscard]] std::optional<std::string> get_active_endpoint_id() const;

        [[nodiscard]] bool has_remaining_frames() const;

        std::vector<AudioStreamingPartState::Channel> get_audio10ms_per_channel(AudioStreamingPartPersistentDecoder &persistent_decoder);
    };

} // wrtc::interfaces::mtproto
