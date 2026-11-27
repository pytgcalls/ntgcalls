//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <optional>
#include <ntgcalls/media/probe/audio_info.hpp>
#include <ntgcalls/media/probe/video_info.hpp>

namespace ntgcalls::media {

    struct MediaInfo {
        std::string container;
        int64_t duration_ms = 0;
        std::optional<AudioInfo> audio;
        std::optional<VideoInfo> video;
    };

} // ntgcalls::media
