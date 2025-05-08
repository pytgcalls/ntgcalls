//
// Created by Laky64 on 13/04/25.
//

#pragma once

#include <vector>
#include <variant>
#include <wrtc/interfaces/mtproto/audio_streaming_part.hpp>
#include <wrtc/interfaces/mtproto/video_streaming_part.hpp>

namespace wrtc {

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
                int32_t channelId = 0;

                Video(const int32_t channelId, const Quality quality) : quality(quality), channelId(channelId) {}
            };

            std::optional<bytes::binary> data;
            Status status = Status::NotReady;
            int64_t minRequestTimestamp = 0;
            int64_t timestampMilliseconds = 0;
            std::variant<Audio, Video, Unified> typeData;

            explicit Part(const std::variant<Audio, Video, Unified> typeData) : typeData(typeData) {}
        };

        struct Video {
            Quality quality;
            std::unique_ptr<VideoStreamingPart> part;
            double lastFramePts = -1.0;
            bool isPlaying = false;
            std::unique_ptr<Part> qualityUpdatePart;
        };

        AudioStreamingPartPersistentDecoder audioDecoder;
        int64_t timestamp = 0;
        int64_t duration = 0;
        Status status = Status::Pending;
        std::vector<std::unique_ptr<Part>> parts;
        std::unique_ptr<AudioStreamingPart> audio;
        std::vector<std::unique_ptr<Video>> video;
        std::unique_ptr<VideoStreamingPart> unifiedAudio;
    };

} // wrtc
