//
// Created by Laky64 on 15/04/25.
//

#include <wrtc/interfaces/mtproto/video_streaming_part.hpp>

namespace wrtc {
    VideoStreamingPart::VideoStreamingPart(bytes::binary&& data, webrtc::MediaType mediaType) {
        if (!data.empty()) {
            state = std::make_unique<VideoStreamingPartState>(std::move(data), mediaType);
        }
    }

    VideoStreamingPart::~VideoStreamingPart() {
        state = nullptr;
    }

    std::optional<std::string> VideoStreamingPart::getActiveEndpointId() const {
        return state ? state->getActiveEndpointId() : std::nullopt;
    }

    std::optional<VideoStreamingPartFrame> VideoStreamingPart::getFrameAtRelativeTimestamp(VideoStreamingSharedState* sharedState, const double timestamp) const {
        return state ? state->getFrameAtRelativeTimestamp(sharedState, timestamp) : std::nullopt;
    }

    std::vector<AudioStreamingPartState::Channel> VideoStreamingPart::getAudio10msPerChannel(AudioStreamingPartPersistentDecoder &persistentDecoder) const {
        return state ? state->getAudio10msPerChannel(persistentDecoder) : std::vector<AudioStreamingPartState::Channel>();
    }
} // wrtc