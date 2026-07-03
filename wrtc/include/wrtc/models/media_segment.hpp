//
// Created by Lauren on 13/04/25.
//

#pragma once

#include <variant>
#include <vector>
#include <wrtc/interfaces/mtproto/audio_streaming_part.hpp>
#include <wrtc/interfaces/mtproto/video_streaming_part.hpp>

namespace wrtc::models {

    struct MediaSegment {
        enum class Quality {
            None,
            Thumbnail,
            Medium,
            Full
        };

        enum class Status {
            Pending,
            Ready,
        };

        struct Part {
            enum class Status {
                NotReady,
                ResyncNeeded,
                Downloading,
                Success,
            };

            struct Audio {};

            struct Unified {};

            struct Video {
                Quality quality;
                int32_t channel_id = 0;

                Video(const int32_t channel_id, const Quality quality) : quality(quality), channel_id(channel_id) {}
            };

            std::optional<bytes::binary> data;
            Status status = Status::NotReady;
            int64_t min_request_timestamp = 0;
            int64_t timestamp_milliseconds = 0;
            std::variant<Audio, Video, Unified> type_data;

            explicit Part(const std::variant<Audio, Video, Unified> type_data) : type_data(type_data) {}
        };

        struct Video {
            Quality quality;
            std::unique_ptr<interfaces::mtproto::VideoStreamingPart> part;
            double last_frame_pts = -1.0;
            bool is_playing = false;
            std::unique_ptr<Part> quality_update_part;
        };

        interfaces::mtproto::AudioStreamingPartPersistentDecoder audio_decoder;
        int64_t timestamp = 0;
        int64_t duration = 0;
        Status status = Status::Pending;
        std::vector<std::unique_ptr<Part>> parts;
        std::unique_ptr<interfaces::mtproto::AudioStreamingPart> audio;
        std::vector<std::unique_ptr<Video>> video;
        std::unique_ptr<interfaces::mtproto::VideoStreamingPart> unified_audio;
    };

} // wrtc::models
