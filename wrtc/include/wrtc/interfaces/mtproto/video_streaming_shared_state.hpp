//
// Created by Lauren on 15/04/25.
//

#pragma once
#include <memory>
#include <wrtc/interfaces/mtproto/video_streaming_decoder_state.hpp>

namespace wrtc::interfaces::mtproto {

    class VideoStreamingSharedState {
        std::unique_ptr<VideoStreamingDecoderState> decoder_state_;

    public:
        ~VideoStreamingSharedState();

        void update_decoder_state(const AVCodecParameters* codec_parameters, AVRational pkt_timebase);

        int send_frame(const media::DecodableFrame* frame) const;

        int receive_frame(const media::VideoStreamingAVFrame* frame) const;

        void reset() const;
    };

} // wrtc::interfaces::mtproto
