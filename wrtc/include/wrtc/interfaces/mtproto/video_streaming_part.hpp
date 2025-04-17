//
// Created by Laky64 on 15/04/25.
//

#pragma once
#include <wrtc/interfaces/mtproto/video_streaming_part_state.hpp>

namespace wrtc {

    class VideoStreamingPart {
        std::unique_ptr<VideoStreamingPartState> state;

    public:
        explicit VideoStreamingPart(bytes::binary&& data);

        ~VideoStreamingPart();

        std::optional<std::string> getActiveEndpointId() const;

        std::optional<VideoStreamingPartFrame> getFrameAtRelativeTimestamp(VideoStreamingSharedState *sharedState, double timestamp) const;
    };

} // wrtc
