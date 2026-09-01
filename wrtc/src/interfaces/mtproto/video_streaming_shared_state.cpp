//
// Created by Lauren on 15/04/25.
//

#include <wrtc/interfaces/mtproto/video_streaming_shared_state.hpp>

namespace wrtc::interfaces::mtproto {
    VideoStreamingSharedState::~VideoStreamingSharedState() {
        decoder_state_ = nullptr;
    }

    void VideoStreamingSharedState::update_decoder_state(const AVCodecParameters* codec_parameters, const AVRational pkt_timebase) {
        if (decoder_state_ && decoder_state_->supports_decoding(codec_parameters, pkt_timebase)) {
            return;
        }
        decoder_state_ = VideoStreamingDecoderState::create(codec_parameters, pkt_timebase);
    }

    int VideoStreamingSharedState::send_frame(const media::DecodableFrame* frame) const {
        if (!decoder_state_) {
            return AVERROR(EIO);
        }
        return decoder_state_->send_frame(frame);
    }

    int VideoStreamingSharedState::receive_frame(const media::VideoStreamingAVFrame* frame) const {
        if (!decoder_state_) {
            return AVERROR(EIO);
        }
        return decoder_state_->receive_frame(frame);
    }

    void VideoStreamingSharedState::reset() const {
        if (!decoder_state_) {
            return;
        }
        decoder_state_->reset();
    }
} // wrtc::interfaces::mtproto
