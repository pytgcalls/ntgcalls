//
// Created by Lauren on 27/11/26.
//

#include <filesystem>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/containers/libav/libav_demuxer.hpp>
#include <ntgcalls/media/containers/libav/libav_format.hpp>
#include <ntgcalls/media/containers/libav/libav_probe.hpp>
#include <rtc_base/logging.h>
#include <wrtc/video_factory/passthrough/h264_profile.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

namespace ntgcalls::media::containers {
    MediaInfo LibAvProbe::input(const std::string& path) {
        if (!std::filesystem::is_regular_file(path)) {
            RTC_LOG(LS_ERROR) << "Unable to open the file located at \"" << path << "\"";
            throw FileError("Unable to open the file located at \"" + path + "\"");
        }
        av_log_set_level(AV_LOG_QUIET);
        AVFormatContext* context = nullptr;
        if (avformat_open_input(&context, path.c_str(), nullptr, nullptr) < 0) {
            RTC_LOG(LS_ERROR) << "Unsupported or invalid container: " << path;
            throw FFmpegError("Unsupported or invalid container");
        }
        if (avformat_find_stream_info(context, nullptr) < 0) {
            avformat_close_input(&context);
            throw FFmpegError("Unable to read the container streams");
        }
        MediaInfo info;
        info.container = context->iformat->name;
        if (context->duration != AV_NOPTS_VALUE && context->duration > 0) {
            info.duration_ms = context->duration / (AV_TIME_BASE / 1000);
        }
        for (unsigned int i = 0; i < context->nb_streams; i++) {
            const auto* stream = context->streams[i];
            const auto type = stream->codecpar->codec_type;
            if (type == AVMEDIA_TYPE_AUDIO && !info.audio) {
                info.audio = audio_info(stream);
            } else if (type == AVMEDIA_TYPE_VIDEO && !info.video && !(stream->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
                info.video = video_info(stream);
            }
        }
        avformat_close_input(&context);
        return info;
    }

    MediaInfo LibAvProbe::output(const std::string& path) {
        MediaInfo info;
        const auto format = LibAvFormat::from_path(path);
        if (!format || !av_guess_format(LibAvFormat::output_name(*format), nullptr, nullptr)) {
            return info;
        }
        info.container = LibAvFormat::output_name(*format);
        if (const auto encoder = LibAvFormat::pcm_encoder(*format); encoder != AV_CODEC_ID_NONE) {
            if (avcodec_find_encoder(encoder)) {
                info.audio = AudioInfo{avcodec_get_name(encoder), 0, 0, true, false};
            }
        } else if (*format == BaseMediaDescription::Format::WAV) {
            info.audio = AudioInfo{avcodec_get_name(AV_CODEC_ID_PCM_S16LE), 0, 0, true, false};
        } else {
            info.audio = AudioInfo{avcodec_get_name(AV_CODEC_ID_OPUS), 0, 0, true, true};
        }
        if (LibAvFormat::holds(*format, MediaKind::Video)) {
            info.video = VideoInfo{"", 0, 0, 0, false, true, true};
        }
        return info;
    }

    AudioInfo LibAvProbe::audio_info(const AVStream* stream) {
        const auto* parameters = stream->codecpar;
        return {
            avcodec_get_name(parameters->codec_id),
            static_cast<uint32_t>(parameters->sample_rate),
            static_cast<uint8_t>(parameters->ch_layout.nb_channels),
            parameters->codec_id == AV_CODEC_ID_OPUS || avcodec_find_decoder(parameters->codec_id) != nullptr,
            parameters->codec_id == AV_CODEC_ID_OPUS,
        };
    }

    VideoInfo LibAvProbe::video_info(const AVStream* stream) {
        const auto* parameters = stream->codecpar;
        return {
            avcodec_get_name(parameters->codec_id),
            static_cast<uint16_t>(parameters->width),
            static_cast<uint16_t>(parameters->height),
            stream->avg_frame_rate.den ? av_q2d(stream->avg_frame_rate) : 0,
            parameters->video_delay > 0,
            LibAvDemuxer::video_codec(parameters->codec_id).has_value(),
            LibAvDemuxer::video_codec(parameters->codec_id).has_value() && parameters->video_delay == 0 && (parameters->codec_id != AV_CODEC_ID_H264 || wrtc::video_factory::H264Profile::forwardable(parameters->profile & 0xFF, parameters->field_order == AV_FIELD_PROGRESSIVE || parameters->field_order == AV_FIELD_UNKNOWN)),
        };
    }
} // ntgcalls::media::containers
