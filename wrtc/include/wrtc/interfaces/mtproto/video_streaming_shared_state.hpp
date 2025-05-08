//
// Created by Laky64 on 15/04/25.
//

#pragma once
#include <memory>
#include <wrtc/interfaces/mtproto/video_streaming_decoder_state.hpp>

namespace wrtc {

    class VideoStreamingSharedState {
        std::unique_ptr<VideoStreamingDecoderState> decoderState;

    public:
        ~VideoStreamingSharedState();

        void updateDecoderState(const AVCodecParameters* codecParameters, AVRational pktTimebase);

        int sendFrame(const DecodableFrame* frame) const;

        int receiveFrame(const VideoStreamingAVFrame* frame) const;

        void reset() const;
    };

} // wrtc
