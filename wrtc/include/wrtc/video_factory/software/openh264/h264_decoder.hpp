//
// Created by Laky64 on 05/11/24.
//

#pragma once

#ifndef IS_ANDROID
#include <api/video_codecs/video_decoder.h>
#include <common_video/h264/h264_bitstream_parser.h>
#include <common_video/include/video_frame_buffer_pool.h>

extern "C" {
    #include <libavcodec/avcodec.h>
}

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ptr) const { avcodec_free_context(&ptr); }
};
struct AVFrameDeleter {
    void operator()(AVFrame* ptr) const { av_frame_free(&ptr); }
};

struct ScopedPtrAVFreePacket {
    void operator()(AVPacket* packet) const { av_packet_free(&packet); }
};

typedef std::unique_ptr<AVPacket, ScopedPtrAVFreePacket> ScopedAVPacket;

inline ScopedAVPacket MakeScopedAVPacket() {
    ScopedAVPacket packet(av_packet_alloc());
    return packet;
}

namespace openh264 {

    class H264Decoder final: public webrtc::VideoDecoder {
        enum H264DecoderImplEvent {
            kH264DecoderEventInit = 0,
            kH264DecoderEventError = 1,
            kH264DecoderEventMax = 16,
        };

        std::unique_ptr<AVCodecContext, AVCodecContextDeleter> avContext;
        webrtc::H264BitstreamParser h264BitstreamParser;
        std::unique_ptr<AVFrame, AVFrameDeleter> avFrame;
        webrtc::VideoFrameBufferPool ffmpegBufferPool;
        webrtc::DecodedImageCallback* decodedImageCallback;
        bool hasReportedInit;
        bool hasReportedError;

        void ReportInit();

        void ReportError();

        static int AVGetBuffer2(AVCodecContext* context, AVFrame* avFrame, int flags);

        static void AVFreeBuffer2(void* opaque, uint8_t*);

        bool IsInitialized() const;

    public:
        H264Decoder();

        ~H264Decoder() override;

        bool Configure(const Settings& settings) override;

        int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) override;

        int32_t Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs) override;

        int32_t Release() override;
    };

} // openh264
#endif