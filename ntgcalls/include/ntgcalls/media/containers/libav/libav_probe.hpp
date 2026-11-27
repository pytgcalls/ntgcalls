//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <ntgcalls/media/probe/media_info.hpp>

struct AVStream;

namespace ntgcalls::media::containers {

    class LibAvProbe {
        static AudioInfo audio_info(const AVStream* stream);

        static VideoInfo video_info(const AVStream* stream);

    public:
        static MediaInfo input(const std::string& path);

        static MediaInfo output(const std::string& path);
    };

} // ntgcalls::media::containers
