//
// Created by Laky64 on 22/08/2023.
//

#include "ffmpeg.hpp"

namespace ntgcalls {
    FFmpeg::FFmpeg(MediaDescription desc): desc(desc) {
        std::optional<FFmpegOptions> audioOptions, videoOptions;
        std::string path;
        if (desc.audio && desc.video) {
            if (desc.audio->path == desc.video->path) {
                type = Type::Mixed;
                path = desc.audio->path;
                audioOptions = desc.audio->options;
                videoOptions = desc.video->options;
            } else {
                throw InvalidParams("Mixed audio and video with different paths");
            }
        } else if (desc.audio) {
            type = Type::Audio;
            path = desc.audio->path;
            audioOptions = desc.audio->options;
        } else {
            type = Type::Video;
            path = desc.video->path;
            videoOptions = desc.video->options;
        }
        inputFormatContext = avformat_alloc_context();

        if (avformat_open_input(&inputFormatContext, path.c_str(), nullptr, nullptr) < 0) {
            throw FileError("Error opening input file");
        }

        if (avformat_find_stream_info(inputFormatContext, nullptr)) {
            throw FFmpegError("Could not find stream information");
        }

        uint8_t streamCount = sizeof(inputFormatContext->streams);
        audioId = audioOptions.has_value() ? audioOptions->streamId:-1;
        videoId = videoOptions.has_value() ? videoOptions->streamId:-1;
        if (audioId >= streamCount || videoId >= streamCount) {
            auto id = audioId >= streamCount ? audioId:videoId;
            std::string idType = audioId >= streamCount ? "audio":"video";
            throw FFmpegError("Could not find the " + idType + " stream with the id \"" + std::to_string(id) + "\"");
        }
        if (audioId < 0 && isAudio()) {
            audioId = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        }
        if (videoId < 0 && isVideo()) {
            videoId = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        }
        if (audioId < 0 && videoId < 0) {
            throw FFmpegError("No audio or video streams found");
        }

        /*if (isAudio()) {
            audioInputContext = avcodec_alloc_context3(audioCodec);
            if (avcodec_parameters_to_context(audioInputContext, inputFormatContext->streams[audioId]->codecpar) < 0) {
                throw FFmpegError("Unable to copy audio stream parameters in codec context");
            }
            if (avcodec_open2(audioInputContext, audioCodec, nullptr) < 0) {
                throw FFmpegError("Unable to open audio codec");
            }


        }
        if (isVideo()) {
            videoCodecContext = avcodec_alloc_context3(videoCodec);
            if (avcodec_parameters_to_context(videoCodecContext, inputFormatContext->streams[videoId]->codecpar) < 0) {
                throw FFmpegError("Unable to copy video stream parameters in codec context");
            }
            if (avcodec_open2(videoCodecContext, videoCodec, nullptr) < 0) {
                throw FFmpegError("Unable to open video codec");
            }
        }*/
    }

    bool FFmpeg::isAudio() {
        return type == Type::Audio || type == Type::Mixed;
    }

    bool FFmpeg::isVideo() {
        return type == Type::Video || type == Type::Mixed;
    }

    /*void FFmpeg::convertVideoFrame() {
        while (avcodec_receive_frame(videoCodecContext, frame) >= 0) {
            av_image_alloc(convertedFrame->data, convertedFrame->linesize, desc.video->width, desc.video->height, AV_PIX_FMT_YUV420P, 1);

        }
    }*/

    void FFmpeg::convertAudioFrame() {
        /*while (avcodec_receive_frame(audioCodecContext, frame) >= 0) {
            if (avcodec_send_frame(outputCodecContext, frame) < 0) {
                break;
            }

        }*/
    }

    void FFmpeg::convert() {
        /*AVPacket packet;
        eof = av_read_frame(inputFormatContext, &packet) < 0;
        if (!eof) {
            if (packet.stream_index == audioId) {
                if (avcodec_send_packet(audioCodecContext, &packet) < 0) {
                    return;
                }
                while (avcodec_receive_frame(audioCodecContext, frame) >= 0) {
                    int dataSize = av_samples_get_buffer_size(
                            nullptr,
                            outputCodecContext->channels,
                            frame->nb_samples,
                            AV_SAMPLE_FMT_S16,
                            1
                    );
                }
            }
        }
        av_packet_unref(&packet);*/
    }

    FFmpeg::~FFmpeg() {
        /*av_frame_free(&frame);
        if (isAudio()){
            avcodec_close(audioCodecContext);
            swr_free(&swrContext);
        }
        if (isVideo()) {
            avcodec_close(videoCodecContext);
            sws_freeContext(swsContext);
        }
        avformat_close_input(&inputFormatContext);

        frame = nullptr;
        inputFormatContext = nullptr;

        audioCodec = nullptr;
        swrContext = nullptr;
        audioCodecContext = nullptr;

        videoCodec = nullptr;
        swsContext = nullptr;
        videoCodecContext = nullptr;*/
    }
} // ntgcalls