//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <optional>
#include <vector>
#include <ntgcalls/media/containers/libav/libav_format.hpp>
#include <ntgcalls/media/containers/media_kind.hpp>
#include <ntgcalls/media/containers/muxer.hpp>
#include <ntgcalls/media/containers/track_info.hpp>
#include <ntgcalls/media/media_description.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace ntgcalls::media::containers {

    template<BaseMediaDescription::Format Format>
    class LibAvMuxer final: public Muxer {
        static constexpr int kBufferSize = 64 * 1024;
        static constexpr int kDefaultFrameSamples = 1024;
        static constexpr int64_t kBitratePerChannel = 64000;

        TrackInfo track_;
        AVFormatContext* context_ = nullptr;
        AVIOContext* io_ = nullptr;
        AVStream* stream_ = nullptr;
        AVPacket* packet_ = nullptr;
        AVCodecContext* encoder_ = nullptr;
        AVFrame* frame_ = nullptr;
        AVRational time_base_;
        bytes::binary output_;
        bool header_written_ = false;
        int64_t next_pts_ = 0;
        int64_t first_timestamp_ = 0;
        std::vector<int16_t> pcm_;

        static int write_packet(void* opaque, const uint8_t* buffer, int size);

        void write_header(const EncodedPacket& packet);

        void open_encoder();

        void write(const uint8_t* data, size_t size, int64_t pts, int64_t duration, bool keyframe);

        void encode_pcm(bool all);

        void fill_frame(const int16_t* pcm, int samples) const;

        static AVSampleFormat sample_format(const AVCodec* codec);

        void drain_encoder() const;

        MuxedChunk take_output();

        static AVCodecID codec_id(wrtc::models::Codec codec);

        static constexpr AVCodecID pcm_encoder() {
            return LibAvFormat::pcm_encoder(Format);
        }

        static bytes::binary extradata(const TrackInfo& track, const EncodedPacket& packet);

        static const AVOutputFormat* output_format();

    public:
        explicit LibAvMuxer(const TrackInfo& track);

        ~LibAvMuxer() override;

        LibAvMuxer(const LibAvMuxer&) = delete;

        LibAvMuxer& operator=(const LibAvMuxer&) = delete;

        MuxedChunk add(EncodedPacket packet) override;

        MuxedChunk finish() override;

        static constexpr bool supports(const MediaKind kind) {
            return LibAvFormat::holds(Format, kind);
        }

        static constexpr bool supports(const wrtc::models::Codec codec) {
            switch (Format) {
            case BaseMediaDescription::Format::Ogg:
                return codec == wrtc::models::Codec::Opus;
            case BaseMediaDescription::Format::Matroska:
                return codec == wrtc::models::Codec::Opus || codec == wrtc::models::Codec::H264 || codec == wrtc::models::Codec::VP8 || codec == wrtc::models::Codec::VP9 || codec == wrtc::models::Codec::AV1;
            case BaseMediaDescription::Format::Webm:
                return codec == wrtc::models::Codec::Opus || codec == wrtc::models::Codec::VP8 || codec == wrtc::models::Codec::VP9 || codec == wrtc::models::Codec::AV1;
            case BaseMediaDescription::Format::MP4:
                return codec == wrtc::models::Codec::PCM || codec == wrtc::models::Codec::H264 || codec == wrtc::models::Codec::VP9 || codec == wrtc::models::Codec::AV1;
            case BaseMediaDescription::Format::WAV:
            case BaseMediaDescription::Format::FLAC:
                return codec == wrtc::models::Codec::PCM;
            default:
                return false;
            }
        }
    };

} // ntgcalls::media::containers
