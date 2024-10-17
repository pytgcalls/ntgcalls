//
// Created by Laky64 on 27/09/24.
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
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {

    class VideoSink: public BaseSink {
    protected:
        std::optional<VideoDescription> description;

    public:
        bool setConfig(const std::optional<VideoDescription>& desc);

        std::optional<VideoDescription> getConfig();

        std::chrono::nanoseconds frameTime() override;

        int64_t frameSize() override;
    };

} // ntgcalls
