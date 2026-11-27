//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <deque>
#include <exception>
#include <memory>
#include <vector>
#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/media/codecs/opus_packet_encoder.hpp>
#include <ntgcalls/media/codecs/pcm_resampler.hpp>
#include <ntgcalls/media/containers/demuxer.hpp>
#include <ntgcalls/media/media_description.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace ntgcalls::media::containers {

    class LibAvDemuxer final: public Demuxer {
        static constexpr int kBufferSize = 64 * 1024;

        bool video_;
        BaseMediaDescription::Format format_;
        uint8_t channels_ = 2;
        std::optional<uint32_t> bitrate_;
        ByteSource* source_ = nullptr;
        std::exception_ptr error_;
        bool seekable_ = false;
        AVFormatContext* context_ = nullptr;
        AVIOContext* io_ = nullptr;
        AVCodecContext* decoder_ = nullptr;
        std::unique_ptr<codecs::PcmResampler> resampler_;
        AVPacket* packet_ = nullptr;
        AVFrame* frame_ = nullptr;
        AVStream* stream_ = nullptr;
        wrtc::models::Codec codec_ = wrtc::models::Codec::Opus;
        webrtc::VideoRotation rotation_ = webrtc::kVideoRotation_0;
        std::unique_ptr<codecs::OpusPacketEncoder> encoder_;
        std::vector<int16_t> pcm_;
        std::deque<DemuxedFrame> pending_;
        bool drained_ = false;
        uint8_t nal_length_size_ = 4;
        bool length_prefixed_ = false;
        bytes::binary parameter_sets_;

        static int read_packet(void* opaque, uint8_t* buffer, int size);

        static int64_t seek_packet(void* opaque, int64_t offset, int whence);

        void open();

        void open_container();

        void open_video();

        void open_audio();

        std::optional<DemuxedFrame> next_packet();

        std::optional<DemuxedFrame> next_decoded();

        bool read_stream_packet();

        void receive_frames();

        void encode_pcm(bool all);

        DemuxedFrame video_frame();

        [[nodiscard]] DemuxedFrame audio_frame() const;

        [[noreturn]] void fail(int error, const std::string& message) const;

        [[nodiscard]] std::optional<int64_t> timestamp_ns(int64_t value) const;

        [[nodiscard]] std::optional<std::chrono::nanoseconds> packet_duration() const;

        void parse_avcc(const uint8_t* data, size_t size);

        bytes::binary convert_payload(const uint8_t* data, size_t size, bool keyframe) const;

        static webrtc::VideoRotation stream_rotation(const AVStream* stream);

    public:
        static std::optional<wrtc::models::Codec> video_codec(AVCodecID id);

        explicit LibAvDemuxer(BaseSink* sink);

        ~LibAvDemuxer() override;

        LibAvDemuxer(const LibAvDemuxer&) = delete;

        LibAvDemuxer& operator=(const LibAvDemuxer&) = delete;

        std::optional<DemuxedFrame> next(ByteSource& source) override;
    };

} // ntgcalls::media::containers
