//
// Created by Laky64 on 15/04/25.
//

#include <wrtc/interfaces/mtproto/video_streaming_shared_state.hpp>

namespace wrtc {
    VideoStreamingSharedState::~VideoStreamingSharedState() {
        decoderState = nullptr;
    }

    void VideoStreamingSharedState::updateDecoderState(const AVCodecParameters* codecParameters, const AVRational pktTimebase) {
        if (decoderState && decoderState->supportsDecoding(codecParameters, pktTimebase)) {
            return;
        }
        decoderState = VideoStreamingDecoderState::create(codecParameters, pktTimebase);
    }

    int VideoStreamingSharedState::sendFrame(const DecodableFrame* frame) const {
        if (!decoderState) {
            return AVERROR(EIO);
        }
        return decoderState->sendFrame(frame);
    }

    int VideoStreamingSharedState::receiveFrame(const VideoStreamingAVFrame* frame) const {
        if (!decoderState) {
            return AVERROR(EIO);
        }
        return decoderState->receiveFrame(frame);
    }

    void VideoStreamingSharedState::reset() const {
        if (!decoderState) {
            return;
        }
        decoderState->reset();
    }
} // wrtc