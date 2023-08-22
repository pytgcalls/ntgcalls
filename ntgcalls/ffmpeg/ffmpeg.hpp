//
// Created by Laky64 on 22/08/2023.
//

#pragma once

#include <string>

extern "C" {
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

#include "../exceptions.hpp"
#include "../models/media_description.hpp"

namespace ntgcalls {

    class FFmpeg {
    public:
        enum class Type: int {
            Audio,
            Video,
            Mixed
        };

        FFmpeg(MediaDescription desc);

        ~FFmpeg();

    private:
        MediaDescription desc;
        Type type;
        int8_t audioId, videoId;
        AVFormatContext* inputFormatContext;
        const AVCodec *audioCodec, *videoCodec;
        std::vector<uint8_t> audioData, videoData;
        AVFrame *frame, *convertedFrame;
        AVCodecContext *audioInputContext, *audioOutputContext;
        AVCodecContext *videoInputContext, *videoOutputContext;
        bool eof;

        void convert();

        bool isAudio();

        bool isVideo();

        void convertAudioFrame();
    };

} // ntgcalls
