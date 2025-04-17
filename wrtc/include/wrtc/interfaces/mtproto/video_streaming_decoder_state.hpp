//
// Created by Laky64 on 15/04/25.
//

#pragma once
#include <memory>
#include <wrtc/models/decodable_frame.hpp>
#include <wrtc/models/video_streaming_av_frame.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace wrtc {

    class VideoStreamingDecoderState {
        AVCodecContext* codecContext = nullptr;
        AVCodecParameters* parameters = nullptr;
        AVRational timebase;

        static bool areCodecParametersEqual(const AVCodecParameters& lhs, AVCodecParameters const &rhs);

    public:
        VideoStreamingDecoderState(AVCodecContext *codecContext, const AVCodecParameters *codecParameters, AVRational pktTimebase);

        ~VideoStreamingDecoderState();

        static std::unique_ptr<VideoStreamingDecoderState> create(const AVCodecParameters* codecParameters, AVRational pktTimebase);

        bool supportsDecoding(const AVCodecParameters *codecParameters, AVRational pktTimebase) const;

        int sendFrame(const DecodableFrame* frame) const;

        int receiveFrame(const VideoStreamingAVFrame* frame) const;

        void reset() const;
    };

} // wrtc
