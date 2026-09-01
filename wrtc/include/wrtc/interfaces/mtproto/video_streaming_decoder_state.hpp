//
// Created by Lauren on 15/04/25.
//

#pragma once
#include <memory>
#include <wrtc/interfaces/media/decodable_frame.hpp>
#include <wrtc/interfaces/media/video_streaming_av_frame.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace wrtc::interfaces::mtproto {

    class VideoStreamingDecoderState {
        AVCodecContext* codec_context_ = nullptr;
        AVCodecParameters* parameters_ = nullptr;
        AVRational timebase_;

        static bool are_codec_parameters_equal(const AVCodecParameters& lhs, AVCodecParameters const& rhs);

    public:
        VideoStreamingDecoderState(AVCodecContext* codec_context, const AVCodecParameters* codec_parameters, AVRational pkt_timebase);

        ~VideoStreamingDecoderState();

        static std::unique_ptr<VideoStreamingDecoderState> create(const AVCodecParameters* codec_parameters, AVRational pkt_timebase);

        bool supports_decoding(const AVCodecParameters* codec_parameters, AVRational pkt_timebase) const;

        int send_frame(const media::DecodableFrame* frame) const;

        int receive_frame(const media::VideoStreamingAVFrame* frame) const;

        void reset() const;
    };

} // wrtc::interfaces::mtproto
