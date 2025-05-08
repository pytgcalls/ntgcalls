//
// Created by Laky64 on 15/04/25.
//

#pragma once
#include <optional>
#include <api/media_types.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part.hpp>
#include <wrtc/interfaces/mtproto/video_streaming_part_internal.hpp>

namespace wrtc {

    class VideoStreamingPartState {
        struct StreamEvent {
            int32_t offset = 0;
            std::string endpointId;
            int32_t rotation = 0;
            int32_t extra = 0;
        };

        struct StreamInfo {
            std::string container;
            int32_t activeMask = 0;
            std::vector<StreamEvent> events;
        };

        std::optional<StreamInfo> streamInfo;
        std::vector<VideoStreamingPartFrame> availableFrames;
        std::vector<std::unique_ptr<AudioStreamingPart>> parsedAudioParts;
        std::vector<std::unique_ptr<VideoStreamingPartInternal>> parsedVideoParts;

        static int32_t roundUp(int32_t numToRound);

        static std::optional<int32_t> readInt32(const bytes::binary &data, int &offset);

        static std::optional<uint8_t> readBytesAsInt32(const bytes::binary &data, int &offset, int count);

        static std::optional<std::string> readSerializedString(const bytes::binary &data, int &offset);

        static std::optional<StreamEvent> readVideoStreamEvent(const bytes::binary &data, int &offset);

        static std::optional<StreamInfo> consumeStreamInfo(bytes::binary &data);

    public:
        explicit VideoStreamingPartState(bytes::binary&& data, webrtc::MediaType mediaType);

        ~VideoStreamingPartState();

        std::optional<VideoStreamingPartFrame> getFrameAtRelativeTimestamp(VideoStreamingSharedState *sharedState, double timestamp);

        std::optional<std::string> getActiveEndpointId() const;

        bool hasRemainingFrames() const;

        std::vector<AudioStreamingPartState::Channel> getAudio10msPerChannel(AudioStreamingPartPersistentDecoder &persistentDecoder);
    };

} // wrtc
