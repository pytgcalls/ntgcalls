//
// Created by Lauren on 05/11/24.
//

#pragma once

#ifndef IS_ANDROID
#include <api/video_codecs/video_decoder.h>
#include <common_video/h264/h264_bitstream_parser.h>
#include <common_video/include/video_frame_buffer_pool.h>
#include <wrtc/utils/binary.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ptr) const {
        avcodec_free_context(&ptr);
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* ptr) const {
        av_frame_free(&ptr);
    }
};

struct ScopedPtrAVFreePacket {
    void operator()(AVPacket* packet) const {
        av_packet_free(&packet);
    }
};

typedef std::unique_ptr<AVPacket, ScopedPtrAVFreePacket> ScopedAVPacket;

inline ScopedAVPacket make_scoped_av_packet() {
    ScopedAVPacket packet(av_packet_alloc());
    return packet;
}

namespace openh264 {

    class H264Decoder final: public webrtc::VideoDecoder {
        enum H264DecoderImplEvent {
            H264DecoderEventInit = 0,
            H264DecoderEventError = 1,
            H264DecoderEventMax = 16,
        };

        std::unique_ptr<AVCodecContext, AVCodecContextDeleter> av_context_;
        webrtc::H264BitstreamParser h264_bitstream_parser_;
        std::unique_ptr<AVFrame, AVFrameDeleter> av_frame_;
        webrtc::VideoFrameBufferPool ffmpeg_buffer_pool_;
        webrtc::DecodedImageCallback* decoded_image_callback_;
        bool has_reported_init_;
        bool has_reported_error_;

        void report_init();

        void report_error();

        static int av_get_buffer2(AVCodecContext* context, AVFrame* av_frame, int flags);

        static void av_free_buffer2(void* opaque, bytes::byte*);

        bool is_initialized() const;

    public:
        H264Decoder();

        ~H264Decoder() override;

        bool Configure(const Settings& settings) override;

        int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) override;

        int32_t Decode(const webrtc::EncodedImage& input_image, bool missing_frames, int64_t render_time_ms) override;

        int32_t Release() override;
    };

} // openh264
#endif
