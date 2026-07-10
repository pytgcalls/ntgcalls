//
// Created by Lauren on 27/09/24.
//

#pragma once

// i420 VIDEO CODEC SPECIFICATION
// Frame Time: 1000 / FPS ms
// Max FPS: 60
// Max Height: 1280
// Max Width: 1280
// FrameSize: A YUV frame size for a Width * Height resolution image,
// where Y (luminance) and UV (chrominance) components are combined with a 3:2 pixel ratio.

#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/media/media_description.hpp>

namespace ntgcalls::media {

    class VideoSink: public BaseSink {
    protected:
        std::optional<VideoDescription> description_;

    public:
        bool set_config(const std::optional<VideoDescription>& desc);

        std::optional<VideoDescription> get_config();

        std::chrono::nanoseconds frame_time() override;

        int64_t frame_size() override;

        uint8_t frame_rate() override;
    };

} // ntgcalls::media
